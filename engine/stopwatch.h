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
#include <cstdint>

namespace newsflash
{
    // measure total time for a specific task
    class stopwatch
    {
    public:
        typedef std::chrono::steady_clock clock_t;
        typedef clock_t::period period_t;
        typedef clock_t::time_point point_t;
        typedef clock_t::duration duration_t;

        typedef std::uint32_t ms_t;
        typedef std::uint32_t s_t;
        typedef std::uint64_t us_t;

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

        ms_t ms() const
        {
            std::chrono::milliseconds ms;
            if (paused_)
            {
                ms = std::chrono::duration_cast<std::chrono::milliseconds>(total_);
            }
            else
            {
                const auto& now  = clock_t::now();
                const auto& gone = (now - start_);
                ms = std::chrono::duration_cast<std::chrono::milliseconds>(total_ + gone);                
            }

            return ms.count();
        }

        s_t s() const 
        {
            std::chrono::seconds s;
            if (paused_)
            {
                s = std::chrono::duration_cast<std::chrono::seconds>(total_);
            }
            else
            {
                const auto& now  = clock_t::now();
                const auto& gone = (now - start_);
                s = std::chrono::duration_cast<std::chrono::seconds>(total_ + gone);
            }
            return s.count();
        }
    private:
        duration_t total_;
        point_t start_;
        bool paused_;
    };

} // newsflash
