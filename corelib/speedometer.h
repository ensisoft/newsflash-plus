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
#include <cstddef>
#include <cassert>
#include "stopwatch.h"

namespace corelib
{
    // track data transfer speeds.
    class speedometer 
    {
    public:
        typedef std::chrono::steady_clock clock_t;
        typedef clock_t::time_point point_t;

        speedometer() : speed_(0)
        {}

        void start()
        {
            stamp_ = clock_t::now();
        }

        // submit a sample. samples should be submitted
        // between calls to start() and end().
        double submit(const std::size_t bytes)
        {
            assert(stamp_ != point_t());

            const auto now = clock_t::now();
            const auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(now - stamp_);

            // time elapsed for the sample bytes as a fractional second.
            const double s = ms.count() / 1000.0;

            // this is the last sample speed.
            const double bps = bytes / s;

            // update the current average speed.
            // http://stackoverflow.com/questions/2779600/how-to-estimate-download-time-remaining-accurately
            const double SMOOTHING_FACTOR = 0.05;
            speed_ = SMOOTHING_FACTOR * bps + (1 - SMOOTHING_FACTOR) * speed_;
            stamp_ = now;
            return speed_;
        }

        void end()
        {
            speed_ = 0.0;
            stamp_ = point_t();
        }

        // get the current speed in bytes per second.
        double bps() const
        {
            return speed_;
        }

        bool is_running() const
        {
            return stamp_ != point_t();
        }

    private:
        point_t stamp_;
        double speed_;
    };

} // corelib

