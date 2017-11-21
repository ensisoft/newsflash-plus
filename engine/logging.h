// Copyright (c) 2010-2015 Sami Väisänen, Ensisoft
//
// http://www.ensisoft.com
//
// This software is copyrighted software. Unauthorized hacking, cracking, distribution
// and general assing around is prohibited.
// Redistribution and use in source and binary forms, with or without modification,
// without permission are prohibited.
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#include "newsflash/config.h"

#include <system_error>
#include <fstream>
#include <ostream>
#include <cerrno>
#include <mutex>
#include <vector>

#include "format.h"
#include "utf8.h"

#ifdef NEWSFLASH_ENABLE_LOG
#  define LOG_E(...) WriteLog(newsflash::logevent::error,   __FILE__, __LINE__, ## __VA_ARGS__)
#  define LOG_W(...) WriteLog(newsflash::logevent::warning, __FILE__, __LINE__, ## __VA_ARGS__)
#  define LOG_I(...) WriteLog(newsflash::logevent::info,    __FILE__, __LINE__, ## __VA_ARGS__)
#  define LOG_D(...) WriteLog(newsflash::logevent::debug,   __FILE__, __LINE__, ## __VA_ARGS__)
#  define LOG_FLUSH() FlushThreadLog()
#else
#  define LOG_E(...)  while(false)
#  define LOG_W(...)  while(false)
#  define LOG_I(...)  while(false)
#  define LOG_D(...)  while(false)
#  define LOG_FLUSH() while (false)
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

    // Logger interface for writing log messages to an object
    // such as stdout or file depending on the implementation.
    class Logger
    {
    public:
        virtual ~Logger() = default;

        // write the string message.
        virtual void Write(const std::string& msg) = 0;

        // flush pending changes (if) any.
        virtual void Flush() = 0;

        // returns true if the output stream is open
        virtual bool IsOpen() const = 0;

        // get the log output name
        virtual std::string GetName() const = 0;
    protected:
    private:
    };

    // this logger implementation saves all the log messages into
    // an internal buffer.
    class BufferLogger : public Logger
    {
    public:
        virtual void Write(const std::string& msg) override
        {
            lines_.push_back(msg);
        }
        virtual void Flush() override
        {
            // this is no-op
        }
        virtual bool IsOpen() const override
        { return true; }

        virtual std::string GetName() const override
        { return "BufferLogger"; }

        std::size_t GetNumLines() const
        { return lines_.size(); }

        std::string GetLine(std::size_t i) const
        { return lines_[i]; }

        void Clear()
        { lines_.clear(); }
    private:
        std::vector<std::string> lines_;
    };

    class FileLogger : public Logger
    {
    public:
        FileLogger(const std::string& file, bool ignore_failures)
        {
            out_.open(file, std::ios::trunc);
            if (!out_.is_open() && !ignore_failures)
                throw std::system_error(std::error_code(errno,
                    std::system_category()), "failed to open log file: " + file);

            file_ = file;
        }
        virtual void Write(const std::string& msg) override
        {
            std::lock_guard<std::mutex> lock(mutex_);
            out_ << msg;
        }

        virtual void Flush() override
        {
            std::lock_guard<std::mutex> lock(mutex_);
            out_.flush();
        }

        virtual bool IsOpen() const override
        { return out_.is_open(); }

        virtual std::string GetName() const override
        { return file_; }
    private:
        std::ofstream out_;
        std::string file_;
        std::mutex mutex_;
    };

    class StdLogger : public Logger
    {
    public:
        StdLogger(std::ostream& out) : out_(out)
        {}
        virtual void Write(const std::string& msg) override
        {
            std::lock_guard<std::mutex> lock(mutex_);
            out_ << msg;
        }
        virtual void Flush() override
        {
            std::lock_guard<std::mutex> lock(mutex_);
            out_.flush();
        }

        virtual bool IsOpen() const override
        { return true; }

        virtual std::string GetName() const override
        { return "StdLogger"; }
    private:
        std::ostream& out_;
        std::mutex mutex_;
    };

    Logger* GetThreadLog();
    Logger* SetThreadLog(Logger* logger);

    void FlushThreadLog();
    void EnableDebugLog(bool on_off);

    bool IsDebugLogEnabled();

    template<typename... Args>
    void WriteLog(logevent type, const char* file, int line, const Args&... args)
    {
        if (type == logevent::debug)
        {
            if (!IsDebugLogEnabled())
                return;
        }

        auto* logger = GetThreadLog();
        if (logger == nullptr)
            return;

        std::stringstream ss;
        detail::beg_log_event(ss, type, file, line);
        detail::write_log_args(ss, args...);
        detail::end_log_event(ss);
        logger->Write(ss.str());
    }

} // newsflash
