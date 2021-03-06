/**
 * Copyright 2013-2015 Heiko Burau, Rene Widera, Benjamin Worpitz
 *
 * This file is part of libPMacc.
 *
 * libPMacc is free software: you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libPMacc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with libPMacc.
 * If not, see <http://www.gnu.org/licenses/>.
 */


#include "Environment.hpp"
#include "eventSystem/EventSystem.hpp"

#include "fields/SimulationFieldHelper.hpp"
#include "mappings/kernel/ExchangeMapping.hpp"

#include "particles/memory/boxes/ParticlesBox.hpp"
#include "particles/memory/buffers/ParticlesBuffer.hpp"


namespace PMacc
{
    template<typename T_ParticleDescription, class MappingDesc>
    void ParticlesBase<T_ParticleDescription, MappingDesc>::deleteGuardParticles(uint32_t exchangeType)
    {
        KernelDeleteParticles kernelDeleteParticles;

        ExchangeMapping<GUARD, MappingDesc> mapper(this->cellDescription, exchangeType);
        DataSpace<Dim> blockSize(DataSpace<Dim>::create(1));
        blockSize.x() = static_cast<AlpakaIdxSize>(TileSize);

        __cudaKernel(
            kernelDeleteParticles,
            alpaka::dim::DimInt<MappingDesc::Dim>,
            mapper.getGridDim(),
            blockSize)(
                particlesBuffer->getDeviceParticleBox(),
                mapper);
    }

    template<typename T_ParticleDescription, class MappingDesc>
    template<uint32_t T_area>
    void ParticlesBase<T_ParticleDescription, MappingDesc>::deleteParticlesInArea()
    {
        KernelDeleteParticles kernelDeleteParticles;

        AreaMapping<T_area, MappingDesc> mapper(this->cellDescription);
        DataSpace<Dim> blockSize(DataSpace<Dim>::create(1));
        blockSize.x() = static_cast<AlpakaIdxSize>(TileSize);

        __cudaKernel(
            kernelDeleteParticles,
            alpaka::dim::DimInt<MappingDesc::Dim>,
            mapper.getGridDim(),
            blockSize)(
                particlesBuffer->getDeviceParticleBox(),
                mapper);
    }

    template<typename T_ParticleDescription, class MappingDesc>
    void ParticlesBase<T_ParticleDescription, MappingDesc>::reset(uint32_t )
    {
        deleteParticlesInArea<CORE+BORDER+GUARD>();
        particlesBuffer->reset( );
    }

    template<typename T_ParticleDescription, class MappingDesc>
    void ParticlesBase<T_ParticleDescription, MappingDesc>::bashParticles(uint32_t exchangeType)
    {
        if (particlesBuffer->hasSendExchange(exchangeType))
        {
            KernelBashParticles kernelBashParticles;

            ExchangeMapping<GUARD, MappingDesc> mapper(this->cellDescription, exchangeType);
            DataSpace<Dim> blockSize(DataSpace<Dim>::create(1));
            blockSize.x() = static_cast<AlpakaIdxSize>(TileSize);

            particlesBuffer->getSendExchangeStack(exchangeType).setCurrentSize(0);

            __cudaKernel(
                kernelBashParticles,
                alpaka::dim::DimInt<MappingDesc::Dim>,
                mapper.getGridDim(),
                blockSize)(
                    particlesBuffer->getDeviceParticleBox(),
                    particlesBuffer->getSendExchangeStack(exchangeType).getDeviceExchangePushDataBox(),
                    mapper);
        }
    }

    template<typename T_ParticleDescription, class MappingDesc>
    void ParticlesBase<T_ParticleDescription, MappingDesc>::insertParticles(uint32_t exchangeType)
    {
        if (particlesBuffer->hasReceiveExchange(exchangeType))
        {

            size_t const grid(particlesBuffer->getReceiveExchangeStack(exchangeType).getHostCurrentSize());
            if (grid != 0)
            {
                KernelInsertParticles kernelInsertParticles;

                ExchangeMapping<GUARD, MappingDesc> mapper(this->cellDescription, exchangeType);
                __cudaKernel(
                    kernelInsertParticles,
                    alpaka::dim::DimInt<1u>,
                    static_cast<AlpakaIdxSize>(grid),
                    static_cast<AlpakaIdxSize>(TileSize))(
                        particlesBuffer->getDeviceParticleBox(),
                        particlesBuffer->getReceiveExchangeStack(exchangeType).getDeviceExchangePopDataBox(),
                        mapper);
                }
        }
    }

    template<typename T_ParticleDescription, class MappingDesc>
    EventTask ParticlesBase<T_ParticleDescription, MappingDesc>::asyncCommunication(EventTask event)
    {
        EventTask ret;
        __startTransaction(event);
        Environment<>::get().ParticleFactory().createTaskParticlesReceive(*this);
        ret = __endTransaction();

        __startTransaction(event);
        Environment<>::get().ParticleFactory().createTaskParticlesSend(*this);
        ret += __endTransaction();
        return ret;
    }

} //namespace PMacc
