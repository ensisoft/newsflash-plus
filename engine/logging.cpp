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
    std::map<std::string, std::shared_ptr<std::ofstream>> streams;
    std::ofstream* current_stream;
    std::string current_context;
};

boost::thread_specific_ptr<logger> ThreadLogger;

} // namespace

namespace newsflash
{
namespace detail {

    void beg_log_event(std::ostream& current_stream, const std::string& context, logevent type, const char* file, int line)
    {
        using namespace std;

        const auto& timeval = get_localtime();

        current_stream << setw(2) << setfill('0') << timeval.hours << ":"
               << setw(2) << setfill('0') << timeval.minutes << ":"
               << setw(3) << setfill('0') << timeval.millis << " ";
        current_stream << "[" << context << "] ";
        current_stream << (char)type << " ";
    }

    void end_log_event(std::ostream& current_stream)
    {
        current_stream << std::endl;
    }

    std::ostream& get_current_thread_current_log(std::string& context)
    {
        auto logger = ThreadLogger.get();

        assert(logger->current_stream);

        context = logger->current_context;
        return *logger->current_stream;
    }

} // detail

void open_log(std::string context, std::string file)
{
    if (!ThreadLogger.get())
    {
        ThreadLogger.reset(new logger);
    }

    auto logger = ThreadLogger.get();

    auto it = logger->streams.find(file);
    if (it == std::end(logger->streams))
    {
        auto stream = std::make_shared<std::ofstream>();
        stream->open(file, std::ios::trunc);
        if (!stream->is_open())
            throw std::runtime_error("failed to open log file: " + file);

        it = logger->streams.insert(std::make_pair(context, stream)).first;
    }

    auto& stream = it->second;
    logger->current_stream  = stream.get();
    logger->current_context = context;
}

void flush_log()
{
    auto logger = ThreadLogger.get();
    logger->current_stream->flush();
}

void select_log(const std::string& context)
{
    auto logger = ThreadLogger.get();
    auto it = logger->streams.find(context);

    assert(it != std::end(logger->streams));

    logger->current_stream  = it->second.get();
    logger->current_context = it->first;
}

void close_log()
{
    auto logger = ThreadLogger.get();
    logger->streams.erase(logger->current_context);
    logger->current_stream = nullptr;
    logger->current_context = "";

    if (logger->streams.empty())
        ThreadLogger.reset();

}

} // newsflash
