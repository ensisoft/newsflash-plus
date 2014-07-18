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
#include <queue>
#include "waithandle.h"
#include "socketapi.h"

namespace newsflash
{
    class command;
    class datalink;

    class connection
    {
    public:
        enum class error {
            none, 
            connection_refused, 
            authentication_failed,
            service_temporarily_unavailable,
            service_permanently_unavailable,
            socket, timeout, unknown
        };

        enum class state {
            disconnected, connecting, ready, active, error
        };

        connection();
       ~connection();

        void connect(ipv4addr_t host, port_t port, bool ssl);

        void enqueue(command* cmd);

        void read();

        void write();

        waithandle wait() const;
    private:
        std::unique_ptr<datalink> link_;
        std::queue<command*> pipeline_;
        std::string current_group_;
        error error_;
        state state_;
        bool has_compress_gzip_;
        bool has_xzver_;
        bool has_mode_reader_;
    };

} // newsflash


