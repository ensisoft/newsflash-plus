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

#include <memory>
#include <queue>
#include <cstddef>
#include "cmdqueue.h"
#include "ioqueue.h"
#include "stopwatch.h"

namespace newsflash
{
    class taskcmd;
    class taskio;
    class command;    
    class ioaction;

    // implements the state machinery required to execute tasks
    // send nntp commands and perform IO activity on received data.
    class task 
    {
    public:
        enum class state {
            queued, // currently queued.
            active, // performing IO activity, see iostate
            paused, // paused, waiting for resume
            complete // done.

        };
        enum class error {
            none, damaged, unavailable, error, dmca
        };

        enum class iostate {
            none, preparing, active, waiting, flushing, canceling, finalizing
        };

        enum class warning {
            none, damaged
        };

        struct status {
            task::state state;
            task::error error;
            task::iostate iostate;
            task::warning warning;

        };

        task(std::size_t id, std::size_t account, std::string description,
            std::unique_ptr<taskcmd> cmd, std::unique_ptr<taskio> io);
       ~task();

        // being executing the task. 
        // precondition:  task is in queued state. 
        // postcondition: task is in active state.
        void start(cmdqueue* commands, ioqueue* ioactions);

        // pause the currently executing task. already downloaded
        // commands/buffers are still allowed to perform IO activity
        // but no new commands are generated.
        // precondition: task is in active state. 
        // postcondition: task is in paused state. 
        void pause();

        // resume the currently paused task. 
        // precondition: task is paused
        // postcondition: task is active.
        void resume();

        // flush the task's IO state to the disk.
        // precondition: task is active.
        void flush();

        // update task state with a command response
        void update(std::unique_ptr<command> cmd);

        // update task state with an ioaction response.
        void update(std::unique_ptr<ioaction> io);

        // std::size_t id() const;
        // std::size_t account() const;

    private:
        void schedule();

    private:
        const std::size_t id_;
        const std::string description_;

    private:
        task::state state_;
        task::error error_;

        std::size_t account_;
 //       std::size_t cmd
        std::queue<iostate> iostate_;
        std::unique_ptr<taskcmd> cmds_;
        std::shared_ptr<taskio> io_;

        cmdqueue* cmdqueue_;
        ioqueue* ioqueue_;

        stopwatch clock_;
    };

} // newsflash
