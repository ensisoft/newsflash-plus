// Copyright (c) 2013,2014 Sami Väisänen, Ensisoft 
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

#include <boost/noncopyable.hpp>
#include <condition_variable>
#include <mutex>
#include <deque>
#include "waithandle.h"
#include "event.h"
#include "command.h"

namespace newsflash
{
    class cmdqueue : boost::noncopyable
    {
    public:
        // try to get a command from the queue. 
        // if no commands are in the queue then the 
        // returned pointer is null.
        std::unique_ptr<command> try_get_front()
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (queue_.empty())
                return std::unique_ptr<command> {};

            auto ret = std::move(queue_[0]);
            queue_.pop_front();

            notify_pop();

            return ret;            
        }

        // get a command from the queue. if the queue
        // is empty block untill a command is available.
        std::unique_ptr<command> get_front()
        {
            std::unique_lock<std::mutex> lock(mutex_);

            while (queue_.empty())
                cond_.wait(lock);

            auto ret = std::move(queue_[0]);
            queue_.pop_front();

            notify_pop();

            return ret;            
        }

        // push a command to the front of the queue and
        // transfer the ownership of the cmd to the queue.
        void push_front(std::unique_ptr<command> cmd)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            queue_.push_front(std::move(cmd));

            notify_push();            
        }

        // push a new command to the back of the queue and 
        // transfer the ownership of the cmd to the queue.
        void push_back(std::unique_ptr<command> cmd)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            queue_.push_back(std::move(cmd));

            notify_push();
        }

        template<typename T>
        void push_back(T* cmd)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            queue_.emplace_back(cmd);

            notify_push();
        }

        // get wait handle  for waiting on the queue
        // for new commands.
        waithandle wait() const
        {
            return event_.wait();
        }
    private:
        void notify_push()
        {
            if (queue_.size() == 1)
                event_.set();

            cond_.notify_one();            
        }
        void notify_pop()
        {
            if (queue_.empty())
                event_.reset();            
        }

    private:
        std::condition_variable cond_;
        std::mutex mutex_;
        std::deque<std::unique_ptr<command>> queue_;
        event event_;
    };

} // newsflash


