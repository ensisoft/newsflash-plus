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

// #include <newsflash/workaround.h>

// #include <cassert>
// #include "task_state_machine.h"

// using state  = newsflash::task::state;
// using action = newsflash::task::action;

// namespace newsflash
// {

// task_state_machine::task_state_machine()
// {
//     reset();
// }

// task_state_machine::~task_state_machine()
// {}

// bool task_state_machine::start()
// {
//     if (state_ != task::state::queued)
//         return false;

//     on_start();
//     on_action(action::prepare);
//     goto_state(state::waiting);
//     return true;
// }

// bool task_state_machine::pause()
// {
//     if (state_ != state::queued && state_ != state::active && state_ != state::waiting)
//         return false;

//     if (state_ == state::active)
//         on_stop();

//     goto_state(state::paused);
//     return true;
// }

// bool task_state_machine::resume()
// {
//     if (state_ != state::paused)
//         return false;

//     if (buffers_)
//     {
//         on_start();
//         goto_state(state::waiting);
//     }
//     else
//     {
//         goto_state(state::queued);
//     }
//     return true;
// }

// bool task_state_machine::flush()
// {
//     if (!is_runnable())
//         return false;

//     on_action(action::flush);
//     return true;
// }

// bool task_state_machine::kill()
// {
//     switch (state_)
//     {
//         case state::queued:
//             break;

//         case state::active:
//         case state::waiting:
//             on_stop();
//             invoke(action::cancel);
//             break;

//         case state::debuffering:
//         case state::paused:
//             invoke(action::cancel);
//             break;

//         case state::complete:
//             break;

//         case state::killed:
//             return false;

//     }
//     invoke(action::kill);
//     goto_state(state::killed);
//     return true;
// }

// bool task_state_machine::fault()
// {
//     switch (state_)
//     {
//         case state::queued:
//         case state::killed:
//         case state::complete:
//             return false;

//         case state::active:
//         case state::waiting:
//             on_stop();
//             invoke(action::cancel);
//             break;

//         case state::debuffering:
//             invoke(action::cancel);
//             break;

//         case state::paused:
//             invoke(action::cancel);
//             break;
//     }

//     error_ = true;
//     goto_state(state::complete);    
//     return true;
// }

// bool task_state_machine::disrupt()
// {
//     if (state_ != state::active)
//         return false;

//     goto_state(state::waiting);
//     return true;
// }

// bool task_state_machine::prepare(std::size_t bufcount)
// {
//     assert(!buffers_);

//     buffers_ = bufcount;
//     return true;
// }

// #define MB(x) x * 1024 * 1024

// bool task_state_machine::enqueue(std::size_t bytes)
// {
//     if (!is_runnable())
//         return false;

//     qsize_ += bytes;
//     enqued_++;

//     if (state_ == state::active)
//     {
//         if (enqued_ == buffers_)
//         {
//             on_stop();
//         }
//         else if (qsize_ >= MB(50))
//         {
//             on_stop();
//             goto_state(state::debuffering);
//         }
//     }
//     else if (state_ == state::waiting)
//         goto_state(state::active);

//     return true;
// }

// bool task_state_machine::dequeue(std::size_t bytes)
// {
//     if (!is_runnable())
//         return false;
    
//     assert(qsize_ >= bytes);

//     qsize_  -= bytes;
//     dequed_++;

//     if (state_ == state::active)
//     {
//         if (dequed_ == buffers_)
//         {
//             assert(qsize_ == 0);
//             invoke(action::finalize);
//         }
//     }
//     else if (state_ == state::debuffering)
//     {
//         if (qsize_ == 0)
//         {
//             on_start();
//             goto_state(state::waiting);
//         }
//     }
//     return true;
// }

// bool task_state_machine::complete(task::action action)
// {
//     if (action != task::action::finalize)
//         return false;

//     goto_state(state::complete);
//     return true;
// }

// bool task_state_machine::is_runnable() const
// {
//     switch (state_)
//     {
//         case state::active:
//         case state::waiting:
//         case state::paused:
//         case state::debuffering:
//             return true;
//         default:
//             break;
//     }
//     return false;
// }

// bool task_state_machine::is_active() const
// {
//     switch (state_)
//     {
//         case state::active:
//         case state::waiting:
//             return true;
//         default:
//             break;
//     }
//     return false;
// }

// bool task_state_machine::is_queued() const
// {
//     return state_ == state::queued;
// }

// bool task_state_machine::is_complete() const
// {
//     return state_ == state::complete;
// }


// bool task_state_machine::is_killed() const
// {
//     return state_ == state::killed;
// }

// bool task_state_machine::good() const
// {
//     return !error_;
// }

// task::state task_state_machine::get_state() const
// {
//     return state_;
// }

// void task_state_machine::reset()
// {
//     state_    = state::queued;
//     qsize_    = 0;
//     enqued_   = 0;
//     dequed_   = 0;
//     buffers_  = 0;
//     error_    = false;    
// }

// void task_state_machine::invoke(task::action action)
// {
//     if (on_action)
//         on_action(action);
// }

// void task_state_machine::goto_state(task::state state)
// {
//     assert(state_ != state);

//     if (on_state)
//         on_state(state_, state);

//     state_ = state;
// }

// } // engine
