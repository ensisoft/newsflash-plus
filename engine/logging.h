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
#include <ostream>

#pragma once

#ifdef NEWSFLASH_ENABLE_LOG
#  define LOG_E(...) corelib::write_log(corelib::logevent::error, __FILE__, __LINE__, ## __VA_ARGS__)
#  define LOG_W(...) corelib::write_log(corelib::logevent::warning, __FILE__, __LINE__, ## __VA_ARGS__)
#  define LOG_I(...) corelib::write_log(corelib::logevent::info,  __FILE__, __LINE__, ## __VA_ARGS__)
#  define LOG_D(...) corelib::write_log(corelib::logevent::debug, __FILE__, __LINE__, ## __VA_ARGS__)
#  define LOG_OPEN(file) corelib::open_log(file)
#  define LOG_FLUSH() corelib::flush_log()
#  define LOG_CLOSE() corelib::close_log()
#else
#  define LOG_E(...)
#  define LOG_W(...)
#  define LOG_I(...)
#  define LOG_D(...)
#  define LOG_OPEN(file)
#  define LOG_FLUSH()
#  define LOG_CLOSE();
#endif

namespace corelib
{
    enum class logevent {
        error   = 'E',
        warning = 'W',
        info    = 'I', 
        debug   = 'D'
    };

    template<typename Arg>
    void write_log_args(std::ostream& stream, const Arg& arg)
    {
        stream << arg;
    }

    template<typename Arg, typename... Rest>
    void write_log_args(std::ostream& stream, const Arg& arg, const Rest&... gang)
    {
        stream << arg;
        write_log_args(stream, gang...);
    }

    std::ostream& get_log();

    void begin_log_event(std::ostream& stream, logevent type, const char* file, int line);
    void end_log_event(std::ostream& stream);

    template<typename... Args>
    void write_log(logevent type, const char* file, int line, const Args&... args)
    {
        std::ostream& stream = get_log(); 

        begin_log_event(stream, type, file, line);
        write_log_args(stream, args...);
        end_log_event(stream);
    }

    void open_log(const std::string& filename);
    void flush_log();
    void close_log();

} // corelib
