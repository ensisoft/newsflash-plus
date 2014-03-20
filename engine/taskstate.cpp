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

#include <cassert>
#include "taskstate.h"
#include "utility.h"

namespace newsflash
{

taskstate::taskstate() : state_(state::queued), qsize_(0), curbuf_(0), buffers_(0), error_(false)
{}

taskstate::~taskstate()
{}

bool taskstate::start()
{
    if (state_ != state::queued && state_ != state::paused)
        return false;

    if (state_ == state::queued)
    {
        emit(action::prepare_task);
        emit(action::run_cmd_list);
    }
    else if (state_ == state::paused)
    {
        emit(action::run_cmd_list);
    }
    state_ = state::waiting;
    return true;
}

bool taskstate::pause()
{
    if (state_ == state::paused || state_ == state::complete)
        return false;

    if (state_ == state::active)
        emit(action::stop_cmd_list);

    state_ = state::paused;
    return true;
}

bool taskstate::flush()
{
    if (state_ != state::active)
        return false;

    emit(action::flush_task);
    return true;
}

bool taskstate::cancel()
{
    if (state_ != state::active)
        return false;

    emit(action::cancel_task);
    emit(action::stop_cmd_list);

    state_ = state::complete;    
    return true;
}

void taskstate::enqueue(std::size_t bytes)
{
    qsize_ += bytes;
    if (qsize_ >= MB(50))
    {
        emit(action::stop_cmd_list);
        state_ = state::debuffering;
    }

    if (state_ == state::waiting)
        state_ = state::active;
}

void taskstate::dequeue(std::size_t bytes, bool error)
{
    assert(qsize_);
    assert(qsize_ >= bytes);
    qsize_  -= bytes;
    curbuf_ += 1;

    if (curbuf_ == buffers_)
    {
        assert(qsize_ == 0);
        emit(action::finalize_task);
    }

    if (qsize_ == 0 && state_ == state::debuffering)
    {
        emit(action::run_cmd_list);
    }
}

void taskstate::action_success(taskstate::action action)
{
    if (action == taskstate::action::finalize_task)
        emit(taskstate::action::complete);
}

void taskstate::action_failure(taskstate::action action)
{
    switch (action)
    {
        case taskstate::action::prepare_task:
        break;

        case taskstate::action::flush_task:
        break;

        case taskstate::action::cancel_task:
        break;

        case taskstate::action::finalize_task:
        break;

        default:
            assert(0);        
            break;
    }
    state_ = state::complete;
    error_ = true;
    emit(taskstate::action::complete);
}

bool taskstate::kill()
{
    if (state_ == state::queued || state_ == state::complete)
    {
        emit(action::kill);
        return true;
    }

    emit(action::stop_cmd_list);
    emit(action::cancel_task);
    emit(action::kill);
    return true;
}

void taskstate::emit(taskstate::action action)
{
    on_event(action);
}


} // newsflash
