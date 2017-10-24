// Copyright (c) 2010-2015 Sami Väisänen, Ensisoft
//
// http://www.ensisoft.com
//
// This software is copyrighted software. Unauthorized hacking, cracking, distribution
// and general assing around is prohibited.
// Redistribution and use in source and binary forms, with or without modification,
// without permission are prohibited.
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

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
    struct TaskDesc
    {
        // content errors.
        enum class Errors {

            // some of the requested data was not available
            Unavailable,

            // some the requested data was taken down
            Dmca,

            // some data appears to be damaged
            Damaged,

            Incomplete
        };

        enum class States {
            // initial state
            Queued,

            // was running before but now waiting in the queue
            Waiting,

            // currently being downloaded.
            Active,

            // active but draining queued buffers.
            Crunching,

            // paused by the user
            Paused,

            // task is done. see damaged and error flags for details
            // since there might be content errors.
            Complete,

            // some error occurred making it impossible for the task
            // to continue. examples could be inability to create
            // files in the filesystem.
            Error
        };

        // content error flags if any
        bitflag<Errors> error;

        // current task execution state.
        States state = States::Queued;

        // unique task id.
        std::size_t task_id = 0;

        // the batch id of the task
        std::size_t batch_id = 0;

        // account id that is used for the task
        std::size_t account = 0;

        // the human readable description of the task.
        std::string desc;

        // the path to the data with the task. (if any)
        std::string path;

        // download/transfer size in bytes (if known)
        std::uint64_t size = 0;

        // the time elapsed running the task (in seconds)
        std::uint32_t runtime = 0;

        // the estimated completion time (in seconds),
        // will be set to WHO_KNOWS when eta cannot be calculated
        // for example when task has been paused by the user
        // or is waiting for input.
        std::uint32_t etatime = 0;

        // the current completion %  (range 0.0 - 100.0)
        double completion = 0.0;
    };

} // ui
} // engine
