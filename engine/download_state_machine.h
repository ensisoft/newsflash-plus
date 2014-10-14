// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
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
#include <functional>
#include <cassert>

#include "stopwatch.h"
#include "etacalc.h"
#include "ui/task.h"

namespace newsflash
{
    class download_state_machine 
    {
    public:
        using state = ui::task::state;

        //std::function<void ()> on_start;
        std::function<void ()> on_stop;
        std::function<void ()> on_cancel;
        std::function<void ()> on_complete;
        std::function<void ()> on_activate;
        std::function<void (state old, state now)> on_state_change;

        download_state_machine(std::size_t num_articles) : 
            num_articles_pending_(num_articles), num_actions_(0), state_(state::queued), started_(false)
        {}

       ~download_state_machine()
        {}

        void start() 
        {
            if (state_ != state::queued)
                return;

            goto_state(state::waiting);
            started_ = true;
        }

        // transition to paused state.
        // precondition: task is either queued, waiting or active.
        // postcondition: timers and counters are stopped, task is paused, pending actions are still completed.
        void pause()
        {
            if (state_ == state::paused ||
                state_ == state::complete ||
                state_ == state::error)
                return;

            goto_state(state::paused);
            on_stop();
        }

        // transition from paused to waiting state.
        // precondition: task is paused.
        // postcondition: if task has never run it will become queued, otherwise task will become waiting
        void resume()
        {
            if (state_ != state::paused)
                return;

            if (started_)
            {
                if (!num_articles_pending_ && !num_actions_)
                    goto_state(state::complete);
                else 
                    goto_state(state::waiting);
            }
            else
            {
                goto_state(state::queued);
            }
        }

        // kill the download and transition to terminal null state
        // precondition: any state
        // postcondition: null state object is deleted. if download is non-complete cancel/rollback is performed.
        void kill()
        {
            if (state_ == state::queued || state_ == state::complete)
                return;

            on_cancel();
        }


        void complete_articles(std::size_t num)
        {
            assert(num_articles_pending_ >= num);

            num_articles_pending_ -= num;

            if (!num_articles_pending_ && !num_actions_)
            {
                if (state_ == state::active) 
                {
                    goto_state(state::complete);
                    on_stop();
                    on_complete();
                }
            }
        }

        void activate()
        {
            assert(state_ == state::waiting || state_ == state::active);

            num_actions_++;
            goto_state(state::active);
            on_activate();
        }

        void deactivate()
        {
            assert(num_actions_);          

            if (--num_actions_ == 0)
            {
                if (state_ == state::active)
                {
                    if (!num_articles_pending_)
                        goto_state(state::complete);
                    else goto_state(state::waiting);
                }
                on_stop();
            }
        }

        state get_state() const 
        { return state_; }

        void error()
        {
            if (state_ == state::queued)
                return;

            if (state_ == state::active)
                on_stop();

            on_cancel();
            goto_state(state::error);
        }

    private:
        void goto_state(state s) 
        {
            if (on_state_change)
                on_state_change(state_, s);
            state_ = s;
        }

    private:
        std::size_t num_articles_pending_;
        std::size_t num_actions_;
    private:
        state state_;
    private:
        bool started_;
    };

} // newsflash<2