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

#include "task.h"
#include "taskio.h"
#include "taskcmd.h"
#include "waithandle.h"
#include "ioaction.h"

namespace newsflash
{

task::task(std::size_t id, 
           std::size_t account, 
           std::string description,
           std::unique_ptr<taskcmd> cmd, 
           std::unique_ptr<taskio> io)
    : id_(id)
    , description_(std::move(description))
    , state_(state::queued)
    , error_(error::none)
    , account_(account)
    , cmds_(std::move(cmd))
    , io_(std::move(io))
    , cmdqueue_(nullptr)
    , ioqueue_(nullptr)
{
    iostate_.push(iostate::none);
}

task::~task()
{
    // if the task hasn't completed yet
    // we're going to rollback the changes as a final action.
    if (state_ != state::complete)
    {
        if (ioqueue_)
            ioqueue_->push_back(new io_cancel(0, id_, io_));
    }
}



void task::start(cmdqueue* commands, ioqueue* ioactions)
{
    cmdqueue_ = commands;
    ioqueue_  = ioactions;

    if (state_ == state::queued)
    {
        ioqueue_->push_back(new io_prepare(0, id_, io_));
        iostate_.push(iostate::preparing);
    }
    state_ = state::active;


    clock_.start();

}

void task::pause()
{
    state_ = state::paused;
    clock_.pause();
}

void task::resume()
{
    state_ = state::active;
    clock_.start();
}

void task::flush()
{

}



void task::update(std::unique_ptr<command> cmd)
{

}

void task::update(std::unique_ptr<ioaction> io)
{
    assert(io->task() == id_);
    iostate_.pop();
    try
    {
        io->verify();

        return;
    }
    catch (const std::system_error& e)
    {

    }
    catch (const std::exception& e)
    {

    }
    state_ = state::complete;
}

void task::schedule()
{
    switch (state_)
    {

    }
}

} // newsflash
