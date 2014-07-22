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
#include <cassert>
#include "connection.h"
#include "tcpsocket.h"
#include "sslsocket.h"
#include "socket.h"
#include "utf8.h"

namespace newsflash
{

connection::connection(const std::string& logfile) : error_(error::none), state_(state::disconnected)
{
    LOG_OPEN(logfile);
}

connection::~connection()
{
    LOG_FLUSH();
    LOG_CLOSE();
}

void connection::connect(ipv4addr_t host, port_t port, bool ssl)
{
    if (ssl)
        socket_.reset(new sslsocket());
    else socket_.reset(new  tcpsocket());
    socket_->begin_connect(host, port);

    state_ = state::connecting;

}

// void connection::enqueue(command* cmd)
// {
//     try
//     {
//     }
//     catch (const socket::tcp_exception& e)
//     {}
//     catch (const socket::ssl_exception& e)
//     {}
//     catch (const std::exception& e)
//     {}
// }

void connection::read()
{
    switch (state_)
    {
        case state::connecting:
            socket_->complete_connect();
            
            break;

        case state::active:
        break;

        default:
           assert(0);
           break;
    }
}

void connection::open_log(const std::string& filename)
{
#if defined(WINDOWS_OS)
    const auto& wide = utf8::decode(filename);
    log_.open(wide.c_str());
#else
    log_.open(filename.c_str());
#endif
}

void connection::flush_log()
{
    log_.flush();
}

void connection::close_log()
{
    log_.close();
}

} // newsflash
