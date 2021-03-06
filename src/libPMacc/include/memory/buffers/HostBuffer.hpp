/**
 * Copyright 2013-2015 Rene Widera, Benjamin Worpitz
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

#include "memory/buffers/Buffer.hpp"
#include "dimensions/DataSpace.hpp"

namespace PMacc
{

    class EventTask;

    template <class TYPE, unsigned DIM>
    class DeviceBuffer;

    /**
     * Interface for a DIM-dimensional Buffer of type TYPE on the host
     *
     * @tparam TYPE datatype for buffer data
     * @tparam DIM dimension of the buffer
     */
    template <class TYPE, unsigned DIM>
    class HostBuffer : public Buffer<TYPE, DIM>
    {
    protected:
        using DataViewHost = alpaka::mem::view::View<
            AlpakaHostDev,
            TYPE,
            alpaka::dim::DimInt<DIM>,
            AlpakaSize>;

    protected:
        /**
         * Constructor.
         *
         * @param dataSpace size of each dimension of the buffer
         */
        HostBuffer(DataSpace<DIM> dataSpace) :
            Buffer<TYPE, DIM>(dataSpace, (DIM==1))
        {}

    public:

        /**
         * Destructor.
         */
        virtual ~HostBuffer()
        {
        };

        /**
         * Returns the internal alpaka buffer.
         *
         * @return internal alpaka buffer
         */
        virtual DataViewHost const & getMemBufView() const = 0;
        virtual DataViewHost & getMemBufView() = 0;

        /**
         * Copies the data from the given DeviceBuffer to this HostBuffer.
         *
         * @param other DeviceBuffer to copy data from
         */
        virtual void copyFrom(DeviceBuffer<TYPE, DIM>& other) = 0;

        /*! returns the current size (count of elements)
         * @return current size
         */
        size_t getCurrentSize()
        {
            return this->getSizeHost();
        }

        /*! sets the current size (count of elements)
         * @param newsize new current size
         */
        void setCurrentSize(const size_t newsize)
        {
            this->setSizeHost(newsize);
        }
    };

} //namespace PMacc
