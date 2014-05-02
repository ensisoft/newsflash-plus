// Copyright (c) 2014 Sami Väisänen, Ensisoft 
//
// http://www.ensisoft.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.            

#pragma once

#include <cstddef>
#include <string>

namespace engine
{
    // completion statuses
    enum class task_error {

        // succesfully completed without any errors.
        none,

        // the requested data was not available
        unavailable,

        // the requested data was taken down
        dmca,

        // local system error has occurred 
        // for example a file cannot be created because of insufficient
        // permissions or storage space is exhausted.
        system,
    };

        // execution states
    enum class task_state {

        // the task is currently queued in the engine
        // and will be scheduled for execution at some point.
        queued,

        // task was being run before, but now it's waiting
        // for input data. (for example connection went down,
        // or there are no free connections).
        waiting,

        // task was paused by the user.
        paused,

        // succesfully complete.
        // check the completion status for conditions.
        complete

    };

    // a single task that the engine is performing, such as download
    // or header update.
    struct task
    {
        // error if any
        task_error error;

        // current task execution state.
        task_state state;

        // unique task id.
        std::size_t id;

        // the account to which the task is connected.
        std::size_t account;

        // the human readable description of the task.
        std::string description;

        // download/transfer size in bytes (if known)
        std::uint64_t size;

        // the time (in seconds) elapsed running the task. 
        int runtime; 

        // the estimated completion time (in seconds), or -1 if not known.
        int eta;

        // the current completion %  (range 0.0 - 100.0)
        double completion;

        bool damaged;

    };

} // engine
