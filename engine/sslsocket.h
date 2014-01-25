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

#include <boost/noncopyable.hpp>
#include <openssl/ssl.h>
#include "socket.h"

namespace newsflash
{
    class sslsocket : public socket, boost::noncopyable
    {
    public:
        sslsocket();
       ~sslsocket();

       virtual void connect(ipv4addr_t host, port_t port) override;

       virtual native_errcode_t complete_connect() override;

       virtual void sendall(const void* buff, int len) override;

       virtual int sendsome(const void* buff, int len) override;

       virtual int recvsome(void* buff, int capacity) override;

       virtual void close() override;

       virtual waithandle wait() const override;

       virtual waithandle wait(bool waitread, bool waitwrite) const override;
    private:
        enum class state {
            nothing, connecting, ready
        };

        // actual socket handle
        native_socket_t socket_;

        // wait handle for the socket
        native_handle_t handle_;

        // socket state
        state state_;

        // SSL state
        SSL* ssl_;

        // SSL IO object
        BIO* bio_;
    }; 
} // newsflash

