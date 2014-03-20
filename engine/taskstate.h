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

#include <functional>
#include <cstddef>

namespace newsflash
{
    // task state machine. 
    // taskstate does not implement any actions related
    // to state changes but only the logic regarding
    // state changes themselves. the implementation of 
    // the actual actions depends upon the caller and the
    // callback handler.
    class taskstate 
    {
    public:
        enum class state {
            queued, active, waiting, debuffering, paused, complete
        };

        enum class action {
            prepare_task,   
            flush_task,
            cancel_task,
            finalize_task,
            run_cmd_list,
            stop_cmd_list,
            kill,
            complete
        };

        // callback for performing state change related actions.
        std::function<void (action a)> on_event;

        taskstate();
       ~taskstate();

        // all the state transition functions return true 
        // if the transition is allowed in the current state
        // or false if not allowed.

        // transition to started state.
        // precondition:  queued or paused.
        // postcondition: waiting 
        bool start();

        // transition to paused state.
        // precondition:  state is queued or active
        // postcondition: paused
        bool pause();

        // flush io.
        // precondition:  task is active.
        // postcondition: no state change.
        bool flush();

        // cancel io.
        // precondition:  task is active
        // postcondition: complete
        bool cancel();

        // enqueue a buffer with the specified byte size.
        // this function should be called when a new buffer of data
        // is available for the task.
        // each call to enqueue signals the arrival of one new buffer of data.
        // calls to this function may trigger state changes and actions.
        // each call to enqueue should have a corresponding dequeue call.
        void enqueue(std::size_t bytes);

        // dequeue a buffer with the specified byte size.
        // this function should be called when a buffer has been processed by the task.
        // calls to this function may trigger state changes and actions.
        void dequeue(std::size_t bytes, bool error);

        // call this function as a response to a performed action
        // when the action is succesful.
        void action_success(taskstate::action action);

        // call this function as a response to a performed action
        // when the action has failed.
        void action_failure(taskstate::action action);

        //
        bool kill();
    private:
        void emit(taskstate::action action);

    private:
        std::size_t qsize_;
        std::size_t curbuf_;
        std::size_t buffers_;
        state state_;
        bool error_;

    };

} // newsflash
