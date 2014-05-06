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

#include <boost/test/minimal.hpp>
#include <deque>
#include "../taskstate.h"

using namespace engine;

#define pass(x, s) \
    BOOST_REQUIRE(x == true); \
    BOOST_REQUIRE(state.get_state() == s)

#define fail(x, s) \
    BOOST_REQUIRE(x == false); \
    BOOST_REQUIRE(state.get_state() == s)

const auto queued   = taskstate::state::queued;
const auto active   = taskstate::state::active;
const auto waiting  = taskstate::state::waiting;
//const auto debuf    = taskstate::state::debuffering;
const auto paused   = taskstate::state::paused;
const auto complete = taskstate::state::complete;
const auto killed   = taskstate::state::killed;


#define pause   state.pause()
#define start   state.start()
#define resume  state.resume()
#define flush   state.flush()
#define kill    state.kill()
#define fault   state.fault()
#define disrupt state.disrupt()
#define enqueue state.enqueue(1)
#define dequeue state.dequeue(1)
#define prepare(x) state.prepare(x)
#define complete(x) state.complete(x)

void test_state_changes()
{
    // initial state
    {
        taskstate state;
        BOOST_REQUIRE(state.get_state() == taskstate::state::queued);
    }

    // paused transitions
    {
        taskstate state;
        state.on_action = [](taskstate::action) {};

        // queued -> paused -> queued
        pass(pause, paused);
        pass(resume, queued);

        // queued -> waiting -> paused -> queued
        pass(start, waiting);
        pass(pause, paused);
        pass(resume, queued);

        // queued -> waiting -> active
        pass(start, waiting);
        prepare(10);
        pass(enqueue, active);

        // active -> paused -> waiting
        pass(pause, paused);
        pass(resume, waiting);
    }
    
    // buffering transitions
    {
        taskstate state;
        state.on_action = [](taskstate::action){};

        pass(start, waiting);
        pass(prepare(2), waiting);
        pass(enqueue, active);
        pass(disrupt, waiting);
        pass(enqueue, active);
        pass(dequeue, active);
        pass(dequeue, active);
        pass(complete(taskstate::action::finalize), complete);
    }

    // fault transitions
    {
        taskstate state;
        state.on_action = [](taskstate::action){};

        // queued -> fault
        //pass(fault, complete);
        //state.reset();

        // queued -> waiting -> fault
        pass(start, waiting);
        pass(fault, complete);
        state.reset();

        // queued -> waiting -> active -> fault
        pass(start, waiting);
        pass(prepare(2), waiting);
        pass(enqueue, active);
        pass(fault, complete);
        state.reset();

    }

    // kill transitions
    {
        taskstate state;
        state.on_action = [](taskstate::action){};

        // queued -> killed
        pass(kill, killed);
        state.reset();

        // queued -> paused -> killed
        pass(pause, paused);
        pass(kill, killed);
        state.reset();

        // queued -> waiting -> killed
        pass(start, waiting);
        pass(kill, killed);
        state.reset();

        // queued -> waiting -> active -> killed
        pass(start, waiting);
        pass(prepare(2), waiting);
        pass(enqueue, active);
        pass(kill, killed);
        state.reset();

        // queued -> waiting -> active -> complete -> killed
        pass(start, waiting);
        pass(prepare(1), waiting);
        pass(enqueue, active);
        pass(dequeue, active);
        pass(complete(taskstate::action::finalize), complete);
        pass(kill, killed);
    }

}

void test_state_actions()
{

    // todo:
}

int test_main(int, char*[])
{
    test_state_changes();
    test_state_actions();
    return 0;
}
