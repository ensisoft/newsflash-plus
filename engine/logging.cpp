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

    // http://www.boost.org/doc/libs/1_51_0/doc/html/thread/thread_local_storage.html#thread.thread_local_storage.thread_specific_ptr
    // Note: on some platforms, cleanup of thread-specific data is not performed 
    // for threads created with the platform's native API. 
    // On those platforms such cleanup is only done for threads that 
    // are started with boost::thread unless boost::on_thread_exit() 
    // is called manually from that thread. 

    boost::thread_specific_ptr<std::ofstream> gLog;

} // namespace

namespace newsflash
{

std::ostream& get_log()
{
    return *gLog.get();
}

void begin_log_event(std::ostream& stream, logevent type, const char* file, int line)
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
    if (!gLog.get())
    {
        gLog.reset(new std::ofstream);
    }

    if (filename == "clog")
    {
        std::ios& base = *gLog.get();
        base.rdbuf(std::clog.rdbuf());
    }
    else
    {
#if defined(WINDOWS_OS)
        const std::wstring& wide = utf8::decode(filename);
        gLog->open(wide.c_str());
#else
        gLog->open(filename.c_str());
#endif
    }
}

void flush_log()
{
    gLog->flush();
}

void close_log()
{
    gLog->flush();
    gLog->close();
    delete gLog.release();
}

} // newsflash
