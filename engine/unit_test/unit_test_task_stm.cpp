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

#include <newsflash/workaround.h>

#include <boost/test/minimal.hpp>
#include <deque>
#include "../task_state_machine.h"

#define PASS(x, s) \
    BOOST_REQUIRE(x == true); \
    BOOST_REQUIRE(stm.get_state() == s)

#define FAIL(x, s) \
    BOOST_REQUIRE(x == false); \
    BOOST_REQUIRE(stm.get_state() == s)

using state = newsflash::task::state;

const auto queued   = state::queued;
const auto active   = state::active;
const auto waiting  = state::waiting;
const auto debuf    = state::debuffering;
const auto paused   = state::paused;
const auto complete = state::complete;
const auto killed   = state::killed;


#define PAUSE       stm.pause()
#define START       stm.start()
#define RESUME      stm.resume()
#define FLUSH       stm.flush()
#define KILL        stm.kill()
#define FAULT       stm.fault()
#define DISRUPT     stm.disrupt()
#define ENQUEUE     stm.enqueue(1)
#define DEQUEUE     stm.dequeue(1)
#define PREPARE(x)  stm.prepare(x)
#define COMPLETE(x) stm.complete(x)

void test_state_changes()
{
    using namespace newsflash;

    // initial state
    {
        task_state_machine stm;

        BOOST_REQUIRE(stm.get_state() == state::queued);
    }

    // paused transitions
    {
        task_state_machine stm;
        stm.on_action = [](task::action) {};
        stm.on_start  = [] {};
        stm.on_stop   = [] {};


        // queued -> paused -> queued
        PASS(PAUSE, paused);
        PASS(RESUME, queued);

        // queued -> waiting -> paused -> queued
        PASS(START, waiting);
        PASS(PAUSE, paused);
        PASS(RESUME, queued);

        // queued -> waiting -> active
        PASS(START, waiting);
        PREPARE(10);
        PASS(ENQUEUE, active);

        // active -> paused -> waiting
        PASS(PAUSE, paused);
        PASS(RESUME, waiting);
    }
    
    // buffering transitions<
    {
        task_state_machine stm;
        stm.on_action = [](task::action) {};
        stm.on_start  = [] {};
        stm.on_stop   = [] {};

        PASS(START, waiting);
        PASS(PREPARE(2), waiting);
        PASS(ENQUEUE, active);
        PASS(DISRUPT, waiting);
        PASS(ENQUEUE, active);
        PASS(DEQUEUE, active);
        PASS(DEQUEUE, active);
        PASS(COMPLETE(task::action::finalize), complete);
    }

    // fault transitions
    {
        task_state_machine stm;
        stm.on_action = [](task::action){};
        stm.on_start  = [] {};
        stm.on_stop   = [] {};

        // queued -> fault
        //pass(fault, complete);
        //state.reset();

        // queued -> waiting -> fault
        PASS(START, waiting);
        PASS(FAULT, complete);
        stm.reset();

        // queued -> waiting -> active -> fault
        PASS(START, waiting);
        PASS(PREPARE(2), waiting);
        PASS(ENQUEUE, active);
        PASS(FAULT, complete);
        stm.reset();

    }

    // kill transitions
    {
        task_state_machine stm;
        stm.on_action = [](task::action){};
        stm.on_start  = [] {};
        stm.on_stop   = [] {};


        // queued -> killed
        PASS(KILL, killed);
        stm.reset();

        // queued -> paused -> killed
        PASS(PAUSE, paused);
        PASS(KILL, killed);
        stm.reset();

        // queued -> waiting -> killed
        PASS(START, waiting);
        PASS(KILL, killed);
        stm.reset();

        // queued -> waiting -> active -> killed
        PASS(START, waiting);
        PASS(PREPARE(2), waiting);
        PASS(ENQUEUE, active);
        PASS(KILL, killed);
        stm.reset();

        // queued -> waiting -> active -> complete -> killed
        PASS(START, waiting);
        PASS(PREPARE(1), waiting);
        PASS(ENQUEUE, active);
        PASS(DEQUEUE, active);
        PASS(COMPLETE(task::action::finalize), complete);
        PASS(KILL, killed);
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
