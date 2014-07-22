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
#include <memory>
#include <deque>
#include <fstream>
#include "waithandle.h"
#include "socketapi.h"
#include "logging.h"

namespace newsflash
{
    class socket;

    class connection
    {
    public:
        enum class error {
            none, 
            connection_refused, 
            authentication_failed,
            service_temporarily_unavailable,
            service_permanently_unavailable,
            socket, timeout, ssl, unknown
        };

        enum class state {
            disconnected, connecting, authenticating, ready, active, error
        };

        connection(const std::string& logfile);
       ~connection();

        void connect(ipv4addr_t host, port_t port, bool ssl);

        //void enqueue(command* cmd);

        void read();

        void write();

        waithandle wait() const;
    private:
        void open_log(const std::string& filename);
        void flush_log();
        void close_log();

        template<typename... Args>
        void write_log(logevent type, const char* file, int line, const Args&... args)
        {
            beg_log_event(log_, type, file, line);
            write_log_args(log_, args...);
            end_log_event(log_);
        }

    private:
        std::unique_ptr<socket> socket_;
        std::ofstream log_;
        error error_;
        state state_;
    };

} // newsflash


