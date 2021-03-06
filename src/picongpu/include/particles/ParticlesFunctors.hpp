/**
 * Copyright 2014-2015 Rene Widera, Marco Garten
 *
 * This file is part of PIConGPU.
 *
 * PIConGPU is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PIConGPU is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PIConGPU.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "types.h"
#include "simulation_defines.hpp"
#include <boost/mpl/if.hpp>
#include "traits/HasFlag.hpp"
#include "fields/Fields.def"
#include "math/MapTuple.hpp"
#include <boost/mpl/plus.hpp>
#include <boost/mpl/accumulate.hpp>

#include "particles/traits/GetIonizer.hpp"

namespace picongpu
{

namespace particles
{

template<typename T_SpeciesName>
struct AssignNull
{
    typedef T_SpeciesName SpeciesName;

    template<typename T_StorageTuple>
    void operator()(T_StorageTuple& tuple)
    {
        tuple[SpeciesName()] = NULL;
    }
};

template<typename T_SpeciesName>
struct CallDelete
{
    typedef T_SpeciesName SpeciesName;

    template<typename T_StorageTuple>
    void operator()(T_StorageTuple& tuple)
    {
        __delete(tuple[SpeciesName()]);
    }
};

template<typename T_SpeciesName>
struct CreateSpecies
{
    typedef T_SpeciesName SpeciesName;
    typedef typename SpeciesName::type SpeciesType;

    template<typename T_StorageTuple, typename T_CellDescription>
    HINLINE void operator()(T_StorageTuple& tuple, T_CellDescription* cellDesc) const
    {
        tuple[SpeciesName()] = new SpeciesType(cellDesc->getGridLayout(), *cellDesc, SpeciesType::FrameType::getName());
    }
};

template<typename T_SpeciesName>
struct CallCreateParticleBuffer
{
    typedef T_SpeciesName SpeciesName;
    typedef typename SpeciesName::type SpeciesType;

    template<typename T_StorageTuple>
    HINLINE void operator()(T_StorageTuple& tuple) const
    {

        typedef typename SpeciesType::FrameType FrameType;

        log<picLog::MEMORY >("mallocMC: free slots for species %3%: %1% a %2%") %
            mallocMC::getAvailableSlots(sizeof (FrameType)) %
            sizeof (FrameType) %
            FrameType::getName();

        tuple[SpeciesName()]->createParticleBuffer();
    }
};

template<typename T_SpeciesName>
struct CallInit
{
    typedef T_SpeciesName SpeciesName;
    typedef typename SpeciesName::type SpeciesType;

    template<typename T_StorageTuple>
    HINLINE void operator()(T_StorageTuple& tuple,
                            FieldE* fieldE,
                            FieldB* fieldB,
                            FieldJ* fieldJ,
                            FieldTmp* fieldTmp) const
    {
        tuple[SpeciesName()]->init(*fieldE, *fieldB, *fieldJ, *fieldTmp);
    }
};

template<typename T_SpeciesName>
struct CallReset
{
    typedef T_SpeciesName SpeciesName;
    typedef typename SpeciesName::type SpeciesType;

    template<typename T_StorageTuple>
    HINLINE void operator()(T_StorageTuple& tuple,
                            const uint32_t currentStep)
    {
        tuple[SpeciesName()]->reset(currentStep);
    }
};

template<typename T_SpeciesName>
struct CallUpdate
{
    typedef T_SpeciesName SpeciesName;
    typedef typename SpeciesName::type SpeciesType;
    typedef typename SpeciesType::FrameType FrameType;

    template<typename T_StorageTuple, typename T_Event>
    HINLINE void operator()(
                            T_StorageTuple& tuple,
                            const uint32_t currentStep,
                            const T_Event eventInt,
                            T_Event& updateEvent,
                            T_Event& commEvent
                            ) const
    {
        typedef typename HasFlag<FrameType, particlePusher<> >::type hasPusher;
        if (hasPusher::value)
        {
            PMACC_AUTO(speciesPtr, tuple[SpeciesName()]);

            __startTransaction(eventInt);
            speciesPtr->update(currentStep);
            commEvent += speciesPtr->asyncCommunication(__getTransactionEvent());
            updateEvent += __endTransaction();
        }
    }
};

/** \struct CallIonization
 * 
 * \brief Tests if species can be ionized and calls the kernel to do that
 *
 * \tparam T_SpeciesName name of particle species that is checked for ionization
 */
template<typename T_SpeciesName>
struct CallIonization
{
    typedef T_SpeciesName SpeciesName;
    typedef typename SpeciesName::type SpeciesType;
    typedef typename SpeciesType::FrameType FrameType;
    /* SelectIonizer will be either the specified one or fallback: None */
    typedef typename GetIonizer<SpeciesType>::type SelectIonizer;

    /** Functor implementation
     *
     * \tparam T_StorageStuple contains info about the particle species
     * \tparam T_CellDescription contains the number of blocks and blocksize
     *                           that is later passed to the kernel
     * \param tuple An n-tuple containing the type-info of multiple particle species
     * \param cellDesc points to logical block information like dimension and cell sizes
     * \param currentStep The current time step
     */
    template<typename T_StorageTuple, typename T_CellDescription>
    HINLINE void operator()(
                        T_StorageTuple& tuple,
                        T_CellDescription* cellDesc,
                        const uint32_t currentStep
                        ) const
    {
        /* only if an ionizer has been specified, this is executed */
        typedef typename HasFlag<FrameType, ionizer<> >::type hasIonizer;
        if (hasIonizer::value)
        {

            /* define the type of the species to be created
             * from inside the ionization model specialization
             */
            typedef typename SelectIonizer::DestSpecies DestSpecies;
            /* alias for pointer on source species */
            PMACC_AUTO(srcSpeciesPtr, tuple[SpeciesName()]);
            /* alias for pointer on destination species */
            PMACC_AUTO(electronsPtr,  tuple[typename MakeIdentifier<DestSpecies>::type()]);

            /* 3-dim vector : number of threads to be started in every dimension */
            DataSpace<simDim> block( MappingDesc::SuperCellSize::toRT());

            /** kernelIonizeParticles
             * \brief calls the ionization model and handles that electrons are created correctly
             *        while cycling through the particle frames
             *
             * kernel call : instead of name<<<blocks, threads>>> (args, ...)
             * "blocks" will be calculated from "this->cellDescription" and "CORE + BORDER"
             * "threads" is calculated from the previously defined vector "block"
             */
            particles::ionization::KernelIonizeParticles kernelIonizeParticles;
            __picKernelArea(
                kernelIonizeParticles,
                alpaka::dim::DimInt<simDim>,
                *cellDesc,
                CORE + BORDER,
                block)(
                    srcSpeciesPtr->getDeviceParticlesBox( ),
                    electronsPtr->getDeviceParticlesBox( ),
                    SelectIonizer(currentStep));

            /* fill the gaps in the created species' particle frames to ensure that only
             * the last frame is not completely filled but every other before is full
             */
            electronsPtr->fillAllGaps();

        }
    }

}; // struct CallIonization

} //namespace particles

} //namespace picongpu
