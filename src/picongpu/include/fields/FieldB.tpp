/**
 * Copyright 2013-2015 Axel Huebl, Heiko Burau, Rene Widera, Felix Schmitt,
 *                     Richard Pausch, Benjamin Worpitz
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

#include <iostream>
#include "simulation_defines.hpp"


#include "fields/FieldB.hpp"

#include "fields/LaserPhysics.hpp"

#include "eventSystem/EventSystem.hpp"
#include "dataManagement/DataConnector.hpp"
#include "mappings/kernel/AreaMapping.hpp"
#include "mappings/kernel/ExchangeMapping.hpp"
#include "memory/buffers/GridBuffer.hpp"

#include "fields/FieldManipulator.hpp"

#include "dimensions/SuperCellDescription.hpp"

#include "FieldE.hpp"

#include "MaxwellSolver/Solvers.hpp"
#include "fields/numericalCellTypes/NumericalCellTypes.hpp"

#include "math/Vector.hpp"

#include <list>

#include <boost/mpl/accumulate.hpp>
#include "particles/traits/GetInterpolation.hpp"
#include "traits/GetMargin.hpp"

namespace picongpu
{

using namespace PMacc;

FieldB::FieldB( MappingDesc cellDescription ) :
SimulationFieldHelper<MappingDesc>( cellDescription ),
fieldE( NULL )
{
    /*#####create FieldB###############*/
    fieldB = new GridBuffer<ValueType, simDim > ( cellDescription.getGridLayout( ) );

    typedef bmpl::accumulate<
        VectorAllSpecies,
        typename PMacc::math::CT::make_Int<simDim, 0>::type,
        PMacc::math::CT::max<bmpl::_1, GetLowerMargin< GetInterpolation<bmpl::_2> > >
        >::type LowerMarginInterpolation;

    typedef bmpl::accumulate<
        VectorAllSpecies,
        typename PMacc::math::CT::make_Int<simDim, 0>::type,
        PMacc::math::CT::max<bmpl::_1, GetUpperMargin< GetInterpolation<bmpl::_2> > >
        >::type UpperMarginInterpolation;

    /* Calculate the maximum Neighbors we need from MAX(ParticleShape, FieldSolver) */
    typedef PMacc::math::CT::max<
        LowerMarginInterpolation,
        GetMargin<fieldSolver::FieldSolver, FIELD_B>::LowerMargin
        >::type LowerMargin;
    typedef PMacc::math::CT::max<
        UpperMarginInterpolation,
        GetMargin<fieldSolver::FieldSolver, FIELD_B>::UpperMargin
        >::type UpperMargin;

    const DataSpace<simDim> originGuard( LowerMargin( ).toRT( ) );
    const DataSpace<simDim> endGuard( UpperMargin( ).toRT( ) );

    /*go over all directions*/
    for ( uint32_t i = 1; i < NumberOfExchanges<simDim>::value; ++i )
    {
        DataSpace<simDim> relativMask = Mask::getRelativeDirections<simDim > ( i );
        /* guarding cells depend on direction
         * for negative direction use originGuard else endGuard (relative direction ZERO is ignored)
         * don't switch end and origin because this is a read buffer and no send buffer
         */
        DataSpace<simDim> guardingCells;
        for ( uint32_t d = 0; d < simDim; ++d )
            guardingCells[d] = ( relativMask[d] == -1 ? originGuard[d] : endGuard[d] );
        fieldB->addExchange( GUARD, i, guardingCells, FIELD_B );
    }

}

FieldB::~FieldB( )
{
    __delete(fieldB);
}

SimulationDataId FieldB::getUniqueId()
{
    return getName();
}

void FieldB::synchronize( )
{
    fieldB->deviceToHost( );
}

void FieldB::syncToDevice( )
{

    fieldB->hostToDevice( );
}

EventTask FieldB::asyncCommunication( EventTask serialEvent )
{

    EventTask eB = fieldB->asyncCommunication( serialEvent );
    return eB;
}

void FieldB::init( FieldE &fieldE, LaserPhysics &laserPhysics )
{

    this->fieldE = &fieldE;
    this->laser = &laserPhysics;

    Environment<>::get().DataConnector().registerData( *this );
}

GridLayout<simDim> FieldB::getGridLayout( )
{

    return cellDescription.getGridLayout( );
}

FieldB::DataBoxType FieldB::getHostDataBox( )
{

    return fieldB->getHostBuffer( ).getDataBox( );
}

FieldB::DataBoxType FieldB::getDeviceDataBox( )
{

    return fieldB->getDeviceBuffer( ).getDataBox( );
}

GridBuffer<FieldB::ValueType, simDim> &FieldB::getGridBuffer( )
{

    return *fieldB;
}

void FieldB::reset( uint32_t )
{
    fieldB->getHostBuffer( ).reset( true );
    fieldB->getDeviceBuffer( ).reset( false );
}

HDINLINE
FieldB::UnitValueType
FieldB::getUnit( )
{
    return UnitValueType( UNIT_BFIELD, UNIT_BFIELD, UNIT_BFIELD );
}

std::string
FieldB::getName( )
{
    return "FieldB";
}

uint32_t
FieldB::getCommTag( )
{
    return FIELD_B;
}

} //namespace picongpu
