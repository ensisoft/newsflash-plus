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

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <boost/thread/tss.hpp>
#include <newsflash/warnpop.h>

#include <iomanip>
#include "platform.h"
#include "logging.h"

namespace {
    struct logger {
        std::ostream* out;
    };

    boost::thread_specific_ptr<logger> threadLogger;
} // namespace

namespace newsflash
{
namespace detail {

    void beg_log_event(std::ostream& current_stream, logevent type, const char* file, int line)
    {
        using namespace std;

        const auto& timeval = get_localtime();

        current_stream << setw(2) << setfill('0') << timeval.hours << ":"
               << setw(2) << setfill('0') << timeval.minutes << ":"
               << setw(3) << setfill('0') << timeval.millis << " ";
        current_stream << (char)type << " ";
    }

    void end_log_event(std::ostream& current_stream)
    {
        current_stream << std::endl;
    }

} // detail

std::ostream* get_thread_log()
{
    if (!threadLogger.get())
        return nullptr;

    return threadLogger->out;
}

std::ostream* set_thread_log(std::ostream* out)
{
    std::ostream* current = nullptr;

    if (!threadLogger.get())
        threadLogger.reset(new logger);
    else current = threadLogger->out;

    threadLogger->out = out;
    return current;
}

} // newsflash
