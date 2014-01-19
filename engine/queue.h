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

#include <newsflash/config.h>

#include <condition_variable>
#include <deque>
#include <mutex>
#include <cassert>
#include "event.h"

namespace newsflash
{
    template<typename T>
    class queue
    {
    public:
        typedef T value_type;

        queue()
        {}

        bool get(T& data)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (buff_.empty())
                return false;

            data = buff_[0];
            buff_.pop_front();
            if (buff_.empty())
                event_.reset();
            return true;
        }

        void push(const T& data)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            const bool was_empty = buff_.empty();

            buff_.push_back(data);

            if (was_empty)
                event_.set();
        }


        void push(T&& data)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            const bool was_empty = buff_.empty();

            buff_.push_back(std::move(data));

            if (was_empty)
                event_.set();
        }

        waithandle wait() const
        {
            return event_.wait();
        }

    private:
        std::deque<T> buff_;
        std::mutex mutex_;
        std::condition_variable cond_;
        event event_;
    };

} // newsflash

