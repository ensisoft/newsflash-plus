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

#include <newsflash/config.h>
#include <string>
#include <cstddef>
#include <cstdint>
#include "../bitflag.h"

namespace newsflash
{
    namespace ui {

    // a single task that the engine is performing, such as download
    // or header update.
    struct task
    {
        // content errors.
        enum class errors {
        
            // some of the requested data was not available
            unavailable,

            // some the requested data was taken down
            dmca,

            // some data appears to be damaged
            damaged
        };

        enum class states {
            queued,

            // waiting in the queue
            waiting,

            // currently being downloaded.
            active,

            // active but draining queued buffers.
            crunching,

            // paused by the user
            paused,


            // task is done. see damaged and error flags for details
            complete,
            
            // some error occurred making it impossible for the task
            // to continue. examples could be inability to create
            // files in the filesystem.
            error
        };

        // content error flags if any
        bitflag<errors> error;

        // current task execution state.
        states state;

        // unique task id.
        std::size_t task_id;

        // the batch id of the task
        std::size_t batch_id;

        // account id
        std::size_t account;

        // the human readable description of the task.
        std::string desc;

        // download/transfer size in bytes (if known)
        std::uint64_t size;

        // the time elapsed running the task (in seconds)
        std::uint32_t runtime; 

        // the estimated completion time (in seconds),
        // will be set to WHO_KNOWS when eta cannot be calculated
        // for example when task has been paused by the user
        // or is waiting for input.
        std::uint32_t etatime;

        // the current completion %  (range 0.0 - 100.0)
        double completion;

        task() : 
            state(states::queued),
            task_id(0),
            batch_id(0),
            account(0),
            size(0),
            runtime(0),
            etatime(0),
            completion(0.0)
        {}
    };
    
} // ui
} // engine
