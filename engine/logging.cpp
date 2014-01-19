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

#include <boost/thread/tss.hpp>

#include <iomanip>
#include "platform.h"
#include "logging.h"
#include "utf8.h"

namespace {
//     thread_local std::ofstream gLog; msvc doesn't have thread_local
    boost::thread_specific_ptr<std::ofstream> gLog;

} // namespace

namespace newsflash
{

std::ostream& get_log()
{
    if (!gLog.get())
    {
        gLog.reset(new std::ofstream);
    }

    return *gLog.get();
}

void begin_log_event(std::ostream& stream, logevent type, const char* file, int line)
{
    using namespace std;

    const auto& timeval = get_localtime();

    stream << setw(2) << setfill('0') << timeval.hours << ":"
           << setw(2) << setfill('0') << timeval.minutes << ":"
           << setw(2) << setfill('0') << timeval.millis;
    stream << (char)type << ":";
}

void end_log_event(std::ostream& stream)
{
    stream << std::endl;
}

void open_log(const std::string& filename)
{
    if (!gLog.get())
    {
        gLog.reset(new std::ofstream);
    }

#if defined(WINDOWS_OS)
    const std::wstring& wide = utf8::decode(filename);
    gLog->open(wide.c_str());
#else
    gLog->open(filename.c_str());
#endif
}

} // newsflash
