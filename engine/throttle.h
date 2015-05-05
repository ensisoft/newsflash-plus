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

#include <newsflash/config.h>
#include <atomic>
#include <mutex>
#include <chrono>
#include <cstddef>
#include <algorithm>
#include <limits>
#include <thread>

namespace newsflash
{ 
    // implement throttling to conserve/limit bandwidth usage
    class throttle
    {
    public:
        throttle() : time_(0), quota_(0), accum_(0), enabled_(false)
        {
            start_ = clock_t::now();
        }

        void enable(bool on_off)
        {
            enabled_ = on_off;
        }

        // enable throttling. 
        // set the speed limit to the given bytes_per_second.
        void set_quota(std::size_t bytes_per_second)
        {
            quota_ = bytes_per_second;
        }

        void accumulate(std::size_t actual, std::size_t quota)
        {
            if (!enabled_)
                return;

            std::lock_guard<std::mutex> lock(mutex_);
            if (actual > quota)
                actual = quota;

            // if actual number of bytes received is less than 
            // the quote that was allowed to be returned 
            // we can return the difference to "avaiable" quota, i.e.
            // reduce accum so far.
            accum_ -= (quota - actual);
        }

        std::size_t give_quota()
        {
            if (!enabled_)
                return std::numeric_limits<std::size_t>::max();

            // at any given moment we have a fraction of the total
            // quota available to be shared between all connections.
            std::lock_guard<std::mutex> lock(mutex_);
            const auto now  = millis();
            const auto gone = now - time_;
            if (gone > 1000)
            {
                accum_ = (gone % 1000) / 1000.0 * quota_;
                time_  = now;
            }
            if (accum_ >= quota_)
                return 0;

            const auto avail = quota_ - accum_;
            const auto chunk = std::min<std::size_t>(avail, 1024);

            accum_ += chunk;
            return chunk;
        }

        bool is_enabled() const 
        { return enabled_; }

        std::size_t get_quota() const 
        { return quota_; }


    private:
        using clock_t = std::chrono::steady_clock;
#if defined(__MSVC__)
        // msvc2013 returns this type from steady_clock::now
        // instead of time_point<steady_clock> and then there are no conversion
        // operators between these two unrelated types.
        using point_t = std::chrono::time_point < std::chrono::system_clock > ;
#else
        using point_t = std::chrono::time_point<clock_t>;
#endif
       
        std::size_t millis() const 
        {
            const auto now  = clock_t::now();
            const auto span = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_);
            const auto tick = span.count();
            return tick;
        }


    private:
        point_t start_;
        std::size_t time_;
        std::size_t quota_;
        std::size_t accum_;
        std::mutex  mutex_;
        std::atomic<bool> enabled_;
    };

} // newsflash
