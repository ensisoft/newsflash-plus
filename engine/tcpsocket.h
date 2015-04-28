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
#include "utility.h"
#include "socket.h"

namespace newsflash
{
    // transfer data over raw TCP socket. 
    class tcpsocket : noncopyable, public socket
    {
    public:
        // construct a non-connected socket.
        tcpsocket();

        // take ownership of the given socket and handle.
        // the socket is expected to be already connected.
        tcpsocket(native_socket_t sock, native_handle_t handle);
        tcpsocket(tcpsocket&& other);
       ~tcpsocket();

        virtual void begin_connect(ipv4addr_t host, ipv4port_t port) override;
        virtual void complete_connect() override;
        virtual void sendall(const void* buff, int len) override;
        virtual int sendsome(const void* buff, int len) override;
        virtual int recvsome(void* buff, int capacity) override;
        virtual void close() override;
        virtual waithandle wait() const override;
        virtual waithandle wait(bool waitread, bool waitwrite) const override;
        virtual bool can_recv() const override;

        tcpsocket& operator=(tcpsocket&& other);
    private:
        // the actual socket
        native_socket_t socket_;

        // event handle associated with the socket
        native_handle_t handle_;
    };

} // newsflash
