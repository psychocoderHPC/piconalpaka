/**
 * Copyright 2013-2015 Felix Schmitt, Rene Widera, Benjamin Worpitz
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

#include "eventSystem/tasks/ITask.hpp"
#include "eventSystem/events/CudaEvent.hpp"

#include <memory>

namespace PMacc
{
    class EventStream;

    /**
     * Abstract base class for all tasks which depend on cuda streams.
     */
    class StreamTask : public ITask
    {
    public:

        /**
         * Constructor
         *
         * @param stream the EventStream this StreamTask will use
         */
        StreamTask();

        /**
         * Destructor.
         */
        virtual ~StreamTask()
        {
        }

        /**
         * Returns the cuda event associated with this task.
         * An event has to be recorded or set before calling this.
         *
         * @return the task's cuda event
         */
        CudaEvent getCudaEvent() const;

        /**
         * Sets the
         *
         * @param cudaEvent
         */
        void setCudaEvent(const CudaEvent& cudaEvent);

        /**
         * Returns if this task is finished.
         *
         * @return true if the task is finished, else otherwise
         */
        inline bool isFinished();

        /**
         * Returns the EventStream this StreamTask is using.
         *
         * @return pointer to the EventStream
         */
        EventStream* getEventStream() const;

        /**
         * Sets the EventStream for this StreamTask.
         *
         * @param newStream new event stream
         */
        void setEventStream(EventStream* newStream);

    protected:

        /**
         * Activates this task by recording an event on its stream.
         */
        inline void activate();


    private:
        mutable EventStream* stream;
        std::unique_ptr<CudaEvent> cudaEvent;
        bool alwaysFinished;
    };

} //namespace PMacc
