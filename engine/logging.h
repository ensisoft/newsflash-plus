
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
#include <mutex>

#pragma once

#ifdef NEWSFLASH_ENABLE_LOG
#  define LOG_E(...) newsflash::write_log(newsflash::logevent::error, __FILE__, __LINE__, ## __VA_ARGS__)
#  define LOG_W(...) newsflash::write_log(newsflash::logevent::warning, __FILE__, __LINE__, ## __VA_ARGS__)
#  define LOG_I(...) newsflash::write_log(newsflash::logevent::info,  __FILE__, __LINE__, ## __VA_ARGS__)
#  define LOG_D(...) newsflash::write_log(newsflash::logevent::debug, __FILE__, __LINE__, ## __VA_ARGS__)
#  define LOG_OPEN(context, file) newsflash::open_log(context, file)
#  define LOG_SELECT(context) newsflash::select_log(context)
#  define LOG_FLUSH() newsflash::flush_log()
#  define LOG_CLOSE() newsflash::close_log()

#else
#  define LOG_E(...)
#  define LOG_W(...)
#  define LOG_I(...)
#  define LOG_D(...)
#  define LOG_OPEN(file)
#  define LOG_FLUSH()
#  define LOG_CLOSE();
#endif

namespace newsflash
{
    enum class logevent {
        error   = 'E',
        warning = 'W',
        info    = 'I', 
        debug   = 'D'
    };

    namespace detail {
        template<typename Arg>
        void write_log_args(std::ostream& stream, const Arg& arg)
        {
            stream << arg;
        }

        inline
        void write_log_args(std::ostream& stream, bool value)
        {
            stream << (value ? "True" : "False");
        }

        template<typename Arg, typename... Rest>
        void write_log_args(std::ostream& stream, const Arg& arg, const Rest&... gang)
        {
            stream << arg;
            write_log_args(stream, gang...);
        }

        void beg_log_event(std::ostream& stream, const std::string& context, logevent type, const char* file, int line);
        void end_log_event(std::ostream& stream);

        std::ostream* get_current_thread_current_log(std::string& context);
    } // detail

    template<typename... Args>
    void write_log(logevent type, const char* file, int line, const Args&... args)
    {
        std::string context;
        auto stream = detail::get_current_thread_current_log(context);
        if (stream == nullptr)
            return;

        detail::beg_log_event(*stream, context, type, file, line);
        detail::write_log_args(*stream, args...);
        detail::end_log_event(*stream);
    }

    void open_log(std::string context, std::string file);
    void select_log(const std::string& context);
    void flush_log();
    void close_log();

} // newsflash
