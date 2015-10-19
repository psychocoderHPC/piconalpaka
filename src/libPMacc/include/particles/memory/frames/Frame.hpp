/**
 * Copyright 2013-2014 Rene Widera
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

#pragma once

#include "math/MapTuple.hpp"
#include "particles/boostExtension/InheritLinearly.hpp"
#include "particles/memory/dataTypes/Particle.hpp"
#include "particles/ParticleDescription.hpp"
#include "particles/frame_types.hpp"
#include "compileTime/conversion/SeqToMap.hpp"
#include "compileTime/conversion/OperateOnSeq.hpp"
#include "compileTime/GetKeyFromAlias.hpp"

#include "traits/HasIdentifier.hpp"
#include "traits/HasFlag.hpp"
#include "traits/GetFlagType.hpp"

#include "types.h"

#include <boost/mpl/contains.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/mpl/find.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits.hpp>
#include <boost/mpl/string.hpp>
#include <boost/mpl/map.hpp>
#include <boost/mpl/list.hpp>

namespace PMacc
{

namespace pmath = PMacc::math;
namespace pmacc = PMacc;

/** Frame is a storage for arbitrary number >0 of Particles with attributes
 *
 * @tparam T_CreatePairOperator unary template operator to create a boost pair
 *                              from single type ( pair<name,dataType> )
 *                              @see MapTupel
 * @tparam T_ValueTypeSeq sequence with value_identifier
 * @tparam T_MethodsList sequence of classes with particle methods
 *                       (e.g. calculate mass, gamma, ...)
 * @tparam T_Flags sequence with identifiers to add flags on a frame
 *                 (e.g. useSolverXY, calcRadiation, ...)
 */
template<typename T_CreatePairOperator,
typename T_ParticleDescription >
struct Frame;

template<typename T_CreatePairOperator,
typename T_ParticleDescription >
struct Frame :
public InheritLinearly<typename T_ParticleDescription::MethodsList>,
protected pmath::MapTuple<typename SeqToMap<typename T_ParticleDescription::ValueTypeSeq, T_CreatePairOperator>::type, pmath::AlignedData>,
public InheritLinearly<
    typename OperateOnSeq<
        typename T_ParticleDescription::FrameExtensionList,
        bmpl::apply1<bmpl::_1, Frame<T_CreatePairOperator,T_ParticleDescription> >
    >::type
>
{
    typedef T_ParticleDescription ParticleDescription;
    typedef typename ParticleDescription::Name Name;
    typedef typename ParticleDescription::SuperCellSize SuperCellSize;
    typedef typename ParticleDescription::ValueTypeSeq ValueTypeSeq;
    typedef typename ParticleDescription::MethodsList MethodsList;
    typedef typename ParticleDescription::FlagsList FlagList;
    typedef typename ParticleDescription::FrameExtensionList FrameExtensionList;
    typedef Frame<T_CreatePairOperator, ParticleDescription> ThisType;
    /* definition of the MapTupel where we inherit from*/
    typedef pmath::MapTuple<typename SeqToMap<ValueTypeSeq, T_CreatePairOperator>::type, pmath::AlignedData> BaseType;

    /* type of a single particle*/
    typedef pmacc::Particle<ThisType> ParticleType;

    /** access the Nth particle*/
    template<typename T_idx>
    HDINLINE ParticleType operator[](const T_idx idx)
    {
        return ParticleType(*this, idx);
    }

    /** access the Nth particle*/
    template<typename T_idx>
    HDINLINE const ParticleType operator[](const T_idx idx) const
    {
        return ParticleType(*const_cast<ThisType *>(this), idx);
    }

    /** access attribute with a identifier
     *
     * @param T_Key instance of identifier type
     *              (can be an alias, value_identifier or any other class)
     * @return result of operator[] of MapTupel
     */
    template<typename T_Key >
    HDINLINE
    auto getIdentifier(const T_Key)
    -> decltype(this->BaseType::operator[](typename GetKeyFromAlias<ValueTypeSeq, T_Key>::type()))
    {
        typedef typename GetKeyFromAlias<ValueTypeSeq, T_Key>::type Key;
        return BaseType::operator[](Key());
    }

    /** const version of method getIdentifier(const T_Key) */
    template<typename T_Key >
    HDINLINE
    auto getIdentifier(const T_Key) const
    -> decltype(this->BaseType::operator[](typename GetKeyFromAlias<ValueTypeSeq, T_Key>::type()))
    {
        typedef typename GetKeyFromAlias<ValueTypeSeq, T_Key>::type Key;
        return BaseType::operator[](Key());
    }

    HINLINE static std::string getName()
    {
        return std::string(boost::mpl::c_str<Name>::value);
    }

};

namespace traits
{

template<typename T_IdentifierName,
typename T_CreatePairOperator,
typename T_ParticleDescription
>
struct HasIdentifier<
PMacc::Frame<T_CreatePairOperator, T_ParticleDescription>,
T_IdentifierName
>
{
private:
    typedef PMacc::Frame<T_CreatePairOperator, T_ParticleDescription> FrameType;
public:
    typedef typename FrameType::ValueTypeSeq ValueTypeSeq;
    /* if T_IdentifierName is void_ than we have no T_IdentifierName in our Sequence.
     * check is also valid if T_Key is a alias
     */
    typedef typename GetKeyFromAlias<ValueTypeSeq, T_IdentifierName>::type SolvedAliasName;

    typedef bmpl::contains<ValueTypeSeq, SolvedAliasName> type;
};

template<typename T_IdentifierName,
typename T_CreatePairOperator,
typename T_ParticleDescription
>
struct HasFlag<
PMacc::Frame<T_CreatePairOperator, T_ParticleDescription>, T_IdentifierName>
{
private:
    typedef PMacc::Frame<T_CreatePairOperator, T_ParticleDescription> FrameType;
    typedef typename GetFlagType<FrameType, T_IdentifierName>::type SolvedAliasName;
    typedef typename FrameType::FlagList FlagList;
public:

    typedef bmpl::contains<FlagList, SolvedAliasName> type;
};

template<typename T_IdentifierName,
typename T_CreatePairOperator,
typename T_ParticleDescription
>
struct GetFlagType<
PMacc::Frame<T_CreatePairOperator, T_ParticleDescription>, T_IdentifierName>
{
private:
    typedef PMacc::Frame<T_CreatePairOperator, T_ParticleDescription> FrameType;
    typedef typename FrameType::FlagList FlagList;
public:

    typedef typename GetKeyFromAlias<FlagList, T_IdentifierName>::type type;
};

} //namespace traits

}//namespace PMacc
