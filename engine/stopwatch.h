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

#include <chrono>

namespace newsflash
{
    // measure total time for a specific task
    class stopwatch
    {
    private:
        typedef std::chrono::steady_clock clock_t;
        typedef clock_t::period period_t;
        typedef clock_t::time_point point_t;
        typedef clock_t::duration duration_t;

    public:
        // automatically start and stop the stopwatch to time something
        struct timer {
            stopwatch& watch;

            timer(stopwatch& w) : watch(w) {
                watch.start();
            }
           ~timer() {
                watch.pause();
            }
        };

        typedef std::chrono::seconds::rep s_t;
        typedef std::chrono::milliseconds::rep ms_t;
        typedef std::chrono::microseconds::rep us_t;

        stopwatch() : total_(0), paused_(true)
        {}

        void pause()
        {
            const auto& now = clock_t::now();
            total_ += (now - start_);
            paused_ = true;
        }

        void start()
        {
            start_ = clock_t::now();
            paused_ = false;
        }

        inline
        ms_t ms() const
        {
            return get_time<std::chrono::milliseconds>();
        }

        inline
        s_t s() const 
        {
            return get_time<std::chrono::seconds>();
        }

        inline
        us_t us() const
        {
            return get_time<std::chrono::microseconds>();
        }

    private:
        template<typename TimeUnit>
        typename TimeUnit::rep get_time() const
        {
            TimeUnit ret;
            if (paused_)
            {
                ret = std::chrono::duration_cast<TimeUnit>(total_);
            }
            else
            {
                const auto& now  = clock_t::now();
                const auto& gone = (now - start_);
                ret = std::chrono::duration_cast<TimeUnit>(total_ + gone);
            }
            return ret.count();
        }        

    private:
        duration_t total_;
        point_t start_;
        bool paused_;
    };

} // newsflash
