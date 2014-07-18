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
#include "datalink.h"
#include "command.h"
#include "socket.h"

namespace newsflash
{

connection::connection() : error_(error::none), state_(state::disconnected)
{}

connection::~connection()
{}

void connection::connect(ipv4addr_t host, port_t port, bool ssl)
{
    link_.reset(new datalink(host, port, ssl));
    state_             = state::connecting;
    has_compress_gzip_ = false;
    has_xzver_         = false;
    has_mode_reader_   = false;
}

void connection::enqueue(command* cmd)
{
    try
    {
        link_->send(cmd);
        pipeline_.push(cmd);
        state_ = state::active;
    }
    catch (const socket::tcp_exception& e)
    {}
    catch (const socket::ssl_exception& e)
    {}
    catch (const std::exception& e)
    {}
}

void connection::read()
{
    switch (state_)
    {
        case state::connecting:
        break;

        case state::active:
        break;

        default:
           assert(0);
           break;
    }
}

} // newsflash
