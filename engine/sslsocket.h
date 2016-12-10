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

#include "newsflash/warnpush.h"
#  include <boost/noncopyable.hpp>
#  include <openssl/ssl.h>
#include "newsflash/warnpop.h"

#include "sslcontext.h"
#include "socket.h"

namespace newsflash
{
    // transfer data over SSL over TCP socket.
    class sslsocket : public socket, boost::noncopyable
    {
    public:
        sslsocket();

        // take ownership of the existing socket and handle
        // objects and try to initialize SSL in client mode.
        // the socket is assumed to be already connected.
        sslsocket(native_socket_t sock, native_handle_t handle);

        // take ownership of the existing socket, handle
        // and SSL objects. The socket is assume to be already
        // connected and associated with the given SSL objects.
        // Also the SSL operation mode is presumed to be already set
        // (SSL_accept vs. SSL_connect).
        sslsocket(native_socket_t sock, native_handle_t handle, SSL* ssl, BIO* bio);
        sslsocket(sslsocket&& other);
       ~sslsocket();

        virtual void begin_connect(ipv4addr_t host, ipv4port_t port) override;
        virtual void complete_connect() override;
        virtual void sendall(const void* buff, int len) override;
        virtual int sendsome(const void* buff, int len) override;
        virtual int recvsome(void* buff, int capacity) override;
        virtual void close() override;
        virtual waithandle wait() const override;
        virtual waithandle wait(bool waitread, bool waitwrite) const override;
        virtual bool can_recv() const override;

       sslsocket& operator=(sslsocket&& other);
    private:
        void ssl_wait_write();
        void ssl_wait_read();
        void complete_secure_connect();

    private:
        // actual socket handle
        native_socket_t socket_;

        // wait handle for the socket
        native_handle_t handle_;

        // SSL state
        SSL* ssl_;

        // SSL IO object
        BIO* bio_;

        sslcontext context_;
    };
} // newsflash

