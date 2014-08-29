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

#include <cassert>
#include <thread>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <map>
#include "platform.h"
#include "logging.h"
#include "utf8.h"

namespace {

struct logger {
    std::shared_ptr<std::ofstream> stream;
};

std::mutex mutex;
std::map<std::string, std::shared_ptr<std::ofstream>> streams;

boost::thread_specific_ptr<logger> ThreadLogger;

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

    std::ostream* get_current_thread_current_log()
    {
        if (!ThreadLogger.get())
            return nullptr;

        return ThreadLogger->stream.get();
    }

} // detail

void open_log(std::string context, std::string file)
{
    if (!ThreadLogger.get())
        ThreadLogger.reset(new logger);

    std::lock_guard<std::mutex> lock(mutex);

    auto it = streams.find(context);
    if (it == std::end(streams))
    {
        auto stream = std::make_shared<std::ofstream>();
        stream->open(file, std::ios::trunc);
        if (!stream->is_open())
            throw std::runtime_error("failed to open log file: " + file);

        it = streams.insert(std::make_pair(context, stream)).first;
    }

    ThreadLogger->stream = it->second;
}

void close_log(const std::string& name)
{
    std::lock_guard<std::mutex> lock(mutex);

    streams.erase(name);
}

void flush_log()
{
    if (!ThreadLogger.get())
        ThreadLogger.reset(new logger);

    if (!ThreadLogger->stream)
        return;

    ThreadLogger->stream->flush();
}

void select_log(const std::string& context)
{
    if (!ThreadLogger.get())
        ThreadLogger.reset(new logger);

    std::lock_guard<std::mutex> lock(mutex);

    const auto it = streams.find(context);

    assert(it != std::end(streams));

    ThreadLogger->stream = it->second;
}



} // newsflash
