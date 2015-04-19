
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

#include <newsflash/config.h>
#include <system_error>
#include <fstream>
#include <ostream>
#include <cerrno>
#include "format.h"
#include "utf8.h"

#ifdef NEWSFLASH_ENABLE_LOG
#  define LOG_E(...) write_log(newsflash::logevent::error, __FILE__, __LINE__, ## __VA_ARGS__)
#  define LOG_W(...) write_log(newsflash::logevent::warning, __FILE__, __LINE__, ## __VA_ARGS__)
#  define LOG_I(...) write_log(newsflash::logevent::info,  __FILE__, __LINE__, ## __VA_ARGS__)
#else
#  define LOG_E(...) while(false)
#  define LOG_W(...) while(false)
#  define LOG_I(...) while(false)
#endif

#ifdef NEWSFLASH_DEBUG
#  define LOG_D(...) write_log(newsflash::logevent::debug, __FILE__, __LINE__, ## __VA_ARGS__)
#else
#  define LOG_D(...) while(false)
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
        inline
        void write_log_args(std::ostream& stream, bool val)
        {
            stream << (val ? "True" : "False");
        }

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

        void beg_log_event(std::ostream& stream, logevent type, const char* file, int line);
        void end_log_event(std::ostream& stream);

    } // detail

    class logger 
    {
    public:
        virtual ~logger() = default;

        virtual void write(const std::string& msg) = 0;

        virtual void flush() = 0;

        virtual bool is_open() const = 0;

        virtual std::string name() const = 0;
    protected:
    private:
    };

    class filelogger : public logger
    {
    public:
        filelogger(std::string file, bool ignore_failures)
        {
            out_.open(file, std::ios::trunc);
            if (!out_.is_open() && !ignore_failures)
                throw std::system_error(std::error_code(errno, 
                    std::system_category()), "failed to open log file: " + file);

            file_ = std::move(file);
        }
        virtual void write(const std::string& msg) override
        {
            out_ << msg;
        }

        virtual void flush() override
        {
            out_.flush();
        }

        virtual bool is_open() const override
        { return out_.is_open(); }

        virtual std::string name() const override
        { return file_; }
    private:
        std::ofstream out_;
        std::string file_;
    };

    class stdlog : public logger
    {
    public:
        stdlog(std::ostream& out) : out_(out)
        {}
        virtual void write(const std::string& msg) override
        { 
            out_ << msg;
        }
        virtual void flush() override
        {
            out_.flush();
        }

        virtual bool is_open() const override
        { return true; }

        virtual std::string name() const override
        { return "stdlog"; }
    private:
        std::ostream& out_;
    };

    logger* get_thread_log();
    logger* set_thread_log(logger* log);

    template<typename... Args>
    void write_log(logevent type, const char* file, int line, const Args&... args)
    {
        auto* log = get_thread_log();
        if (log == nullptr)
            return;

        std::stringstream ss;
        detail::beg_log_event(ss, type, file, line);
        detail::write_log_args(ss, args...);
        detail::end_log_event(ss);
        log->write(ss.str());        
    }

} // newsflash
