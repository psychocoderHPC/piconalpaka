/**
 * Copyright 2013-2015 Axel Huebl, Heiko Burau, Rene Widera, Benjamin Worpitz
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

#include "YeeSolver.def"

#include "types.h"
#include "simulation_defines.hpp"

#include "fields/SimulationFieldHelper.hpp"
#include "dataManagement/ISimulationData.hpp"


#include "simulation_classTypes.hpp"
#include "memory/boxes/SharedBox.hpp"
#include "nvidia/functors/Assign.hpp"
#include "mappings/threads/ThreadCollective.hpp"
#include "memory/boxes/CachedBox.hpp"
#include "dimensions/DataSpace.hpp"
#include <fields/FieldE.hpp>
#include <fields/FieldB.hpp>

#include "fields/FieldManipulator.hpp"
#include "fields/MaxwellSolver/Yee/YeeSolver.kernel"

namespace picongpu
{
namespace yeeSolver
{
using namespace PMacc;


template<class CurlE, class CurlB>
class YeeSolver
{
private:
    typedef MappingDesc::SuperCellSize SuperCellSize;


    FieldE* fieldE;
    FieldB* fieldB;
    MappingDesc cellDescription;

    template<uint32_t AREA>
    void updateE()
    {
        /* Courant-Friedrichs-Levy-Condition for Yee Field Solver: */
        static_assert(
            (SPEED_OF_LIGHT*SPEED_OF_LIGHT*DELTA_T*DELTA_T*INV_CELL2_SUM)<=1.0,
            "Courant Friedrichs Levy condition failure. Check your gridConfig.param file.");

        typedef SuperCellDescription<
                SuperCellSize,
                typename CurlB::LowerMargin,
                typename CurlB::UpperMargin
                > BlockArea;

        KernelUpdateE<BlockArea, CurlB> kernelUpdateE;
        __picKernelArea(
            kernelUpdateE,
            alpaka::dim::DimInt<simDim>,
            cellDescription,
            AREA,
            SuperCellSize::toRT())(
                this->fieldE->getDeviceDataBox(),
                this->fieldB->getDeviceDataBox());
    }

    template<uint32_t AREA>
    void updateBHalf()
    {
        typedef SuperCellDescription<
                SuperCellSize,
                typename CurlE::LowerMargin,
                typename CurlE::UpperMargin
                > BlockArea;

        KernelUpdateBHalf<BlockArea, CurlE> kernelUpdateBHalf;
        __picKernelArea(
            kernelUpdateBHalf,
            alpaka::dim::DimInt<simDim>,
            cellDescription,
            AREA,
            SuperCellSize::toRT())(
                this->fieldB->getDeviceDataBox(),
                this->fieldE->getDeviceDataBox());
    }

public:

    YeeSolver(MappingDesc cellDescription) : cellDescription(cellDescription)
    {
        DataConnector &dc = Environment<>::get().DataConnector();

        this->fieldE = &dc.getData<FieldE > (FieldE::getName(), true);
        this->fieldB = &dc.getData<FieldB > (FieldB::getName(), true);
    }

    void update_beforeCurrent(uint32_t)
    {
        updateBHalf < CORE+BORDER >();
        EventTask eRfieldB = fieldB->asyncCommunication(__getTransactionEvent());

        updateE<CORE>();
        __setTransactionEvent(eRfieldB);
        updateE<BORDER>();
    }

    void update_afterCurrent(uint32_t currentStep)
    {
        FieldManipulator::absorbBorder(currentStep,this->cellDescription, this->fieldE->getDeviceDataBox());
        if (laserProfile::INIT_TIME > float_X(0.0))
            fieldE->laserManipulation(currentStep);

        EventTask eRfieldE = fieldE->asyncCommunication(__getTransactionEvent());

        updateBHalf < CORE> ();
        __setTransactionEvent(eRfieldE);
        updateBHalf < BORDER > ();

        FieldManipulator::absorbBorder(currentStep,this->cellDescription, fieldB->getDeviceDataBox());

        EventTask eRfieldB = fieldB->asyncCommunication(__getTransactionEvent());
        __setTransactionEvent(eRfieldB);
    }
};

} // yeeSolver

} // picongpu
