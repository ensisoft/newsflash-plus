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

#include <iostream>
#include <fstream>
#include <iomanip>
#include <mutex>
#include "platform.h"
#include "logging.h"
#include "utf8.h"

namespace {

std::mutex     logMutex;
std::ofstream  logStream;

} // namespace

namespace newsflash
{

std::ostream& get_global_log()
{
    return logStream;
}

std::mutex& get_global_log_mutex()
{
    return logMutex;
}

void beg_log_event(std::ostream& stream, logevent type, const char* file, int line)
{
    using namespace std;

    const auto& timeval = get_localtime();

    stream << setw(2) << setfill('0') << timeval.hours << ":"
           << setw(2) << setfill('0') << timeval.minutes << ":"
           << setw(3) << setfill('0') << timeval.millis << " ";
    stream << (char)type << " ";
}

void end_log_event(std::ostream& stream)
{
    stream << std::endl;
}

void open_log(const std::string& filename)
{
    if (logStream.is_open())
    {
        logStream.flush();
        logStream.close();
    }

    if (filename == "clog")
    {
        std::ios& base = logStream;
        base.rdbuf(std::clog.rdbuf());
    }
    else
    {
#if defined(WINDOWS_OS)
        const std::wstring& wide = utf8::decode(filename);
        logStream.open(wide.c_str());
#else
        logStream.open(filename.c_str());
#endif
    }
}

void flush_log()
{
    logStream.flush();
}

void close_log()
{
    logStream.flush();
    logStream.close();
}

} // newsflash
