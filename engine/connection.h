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

    class connection
    {
    public:
        enum class error {
            none, 
            connection_refused, 
            authentication_failed,
            forbidden,
            service_temporarily_unavailable,
            service_permanently_unavailable,
            timeout,
            tcp, ssl, nntp,
            other
        };

        // enum class state {
        //     disconnected, 
        //     connecting, 
        //     authenticating, 
        //     ready, active, error
        // };

        connection();
       ~connection();

        void connect(ipv4addr_t host, port_t port, bool ssl);

        void enqueue(command* cmd);

        bool readsocket();

        waithandle get_wait_handle() const;
    private:
        void do_send(const std::string& cmd);
        void do_auth(std::string& user, std::string& pass);

    private:
        struct data;

        class state;
        class connect;
        class initialize;
        class transfer;

    private:
        std::unique_ptr<state> state_;
        std::unique_ptr<data> data_;
    };

} // newsflash


