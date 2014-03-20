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
    if (state_ != state::queued)
        return false;

    emit(action::prepare_task);
    emit(action::run_cmd_list);
    state_ = state::waiting;
    return true;
}

bool taskstate::pause()
{
    if (state_ != state::queued || state_ != state::active)
        return false;

    if (state_ == state::active)
        emit(action::stop_cmd_list);

    state_ = state::paused;
    return true;
}

bool taskstate::resume()
{
    if (state_ != state::paused)
        return false;

    if (buffers_)
    {
        emit(action::run_cmd_list);
        state_ = state::waiting;
    }
    else
    {
        state_ = state::queued;
    }
    return true;
}

bool taskstate::flush()
{
    if (!is_runnable())
        return false;

    emit(action::flush_task);
    return true;
}

bool taskstate::kill()
{
    if (state_ == state::kill)
        return false;

    switch (state_)
    {
        case state::queued:
            break;

        case state::active:
        case state::waiting:
        case state::debuffering:
            emit(action::stop_cmd_list);
            emit(action::cancel_task);
            break;

        case state::paused:
            emit(action::cancel_task);
            break;

        case state::complete:
            break;

        case state::kill:
            assert(0);
            break;

    }
    state_ = state::kill;
    return true;
}

bool taskstate::fault()
{
    // todo:
    return false;
}

bool taskstate::disrupt()
{
    // todo:
    return false;
}

bool taskstate::enqueue(std::size_t bytes)
{
    if (!is_runnable())
        return false;

    qsize_ += bytes;

    if (state_ == state::active)
    {
        if (qsize_ >= MB(50))
        {
            emit(action::stop_cmd_list);
            state_ = state::debuffering;
        }
    }
    else if (state_ == state::waiting)
        state_ = state::active;

    return true;
}

bool taskstate::dequeue(std::size_t bytes)
{
    if (!is_runnable())
        return false;

    assert(qsize_);
    assert(qsize_ >= bytes);
    qsize_  -= bytes;
    curbuf_ += 1;

    if (state_ == state::debuffering)
    {
        if (qsize_ == 0)
        {
            emit(action::run_cmd_list);
            state_ = state::waiting;
        }
    }
    else if (curbuf_ == buffers_)
    {
        assert(qsize_ == 0);
        emit(action::finalize_task);
    }
    return true;
}

bool taskstate::complete(taskstate::action action)
{
    // todo:
    return true;
}

bool taskstate::is_runnable() const
{
    switch (state_)
    {
        case state::active:
        case state::waiting:
        case state::debuffering:
        case state::paused:
            return true;
        default:
            break;
    }
    return false;
}

bool taskstate::is_active() const
{
    switch (state_)
    {
        case state::active:
        case state::waiting:
            return true;
        default:
            break;
    }
    return false;
}

void taskstate::emit(taskstate::action action)
{
    on_event(action);
}




} // newsflash
