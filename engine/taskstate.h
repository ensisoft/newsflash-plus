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

namespace corelib
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
            queued, // initial state
            waiting, // runnable, waiting for data
            active, // expecting to process buffers
            paused, // paused
            complete, // completed possibly with fault
            killed // terminal state
        };

        enum class action {
            prepare,   
            flush,
            cancel,
            finalize,
            run_cmd_list,
            stop_cmd_list
        };

        // callback for performing state related actions.
        std::function<void (taskstate::action action)> on_action;

        // callback for state changes.
        // the callback is invoked *before* the new state is set, so while in the callback
        // functions reading current state will be based on  current state.
        std::function<void (taskstate::state current, taskstate::state next)> on_state_change;

        taskstate();
       ~taskstate();

        // all the state transition functions return true 
        // if the transition is allowed in the current state
        // or false if not allowed.

        // transition to running state
        // precondition:  queued
        // postcondition: waiting
        bool start();

        // transition to paused state.
        // precondition:  state is queued, active or waiting
        // postcondition: paused
        bool pause();

        // transition to running or queued state
        // precondition: paused
        // postcondition: queued or waiting
        bool resume();

        // flush io.
        // precondition:  is_runnable()
        // postcondition: no state change.
        bool flush();

        // kill the task.
        // precondition: not killed
        // postcondition: killed
        bool kill();

        // fail the task with an error.
        // precondition: not (queued || killed || complete)
        // postcondition: complete with error
        bool fault();

        // disrupt task when buffers are expected.
        // precondition: active
        // postcondition: waiting
        bool disrupt();

        // prepare to receive buffers. this should be called only once
        // when the expected number of data buffers becomes known.
        // precondition: waiting
        // postcondition: 
        bool prepare(std::size_t bufcount);

        // enqueue a buffer with the specified byte size.
        // this function should be called when a new buffer of data
        // is available for the task.
        // each call to enqueue signals the arrival of one new buffer of data.
        // calls to this function may trigger state changes and actions.
        // each call to enqueue should have a corresponding dequeue call.
        // 
        // precondition: is_runnable()
        // postcondition: 
        bool enqueue(std::size_t bytes);

        // dequeue a buffer with the specified byte size.
        // this function should be called when a buffer has been processed by the task.
        // calls to this function may trigger state changes and actions.
        //
        // precondition: is_runnable()
        // postcondition:
        bool dequeue(std::size_t bytes);

        // call this function as a response to a performed action
        // when the action is succesful.
        bool complete(taskstate::action action);


        // returns true if state is in runnable states
        // i.e. active, waiting, paused
        bool is_runnable() const;

        // returns true if task is in active states
        // i.e. active or waiting
        bool is_active() const;

        // returns true if task is queued
        bool is_queued() const;

        bool is_complete() const;

        bool is_killed() const;

        // good flag is true if no fault() has occurred, otherwise false.
        bool good() const;

        // overflow flag is true if maximum number of enqueued
        // buffers has been reached.
        bool overflow() const;

        state get_state() const;

        void reset();

    private:
        void emit(taskstate::action action);

    private:
        void goto_state(taskstate::state state);

    private:
        state state_;        
        std::size_t qsize_;
        std::size_t enqued_;
        std::size_t dequed_;
        std::size_t buffers_;
        bool error_;
        bool overflow_;

    };

} // corelib
