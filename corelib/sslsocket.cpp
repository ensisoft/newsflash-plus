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

#define OPENSSL_THREAD_DEFINES

#include <newsflash/config.h>

#if defined(WINDOWS_OS)
#  include <windows.h>
#  include <winsock2.h>
#elif defined(LINUX_OS)
#  include <sys/socket.h>
#endif

#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/opensslconf.h>
#ifndef OPENSSL_THREADS
#  error need openssl with thread support
#endif
#include <mutex>
#include <thread>
#include <atomic>
#include <cassert>
#include "sslsocket.h"
#include "sslcontext.h"
#include "socketapi.h"

// openssl has cryptic meanings for the special error codes SSL_ERROR_WANT_READ and
// SSL_ERROR_WANT_WRITE. Basically what these means the the SSL_read/SSL_read operation
// could not be completed because an SSL transaction is taking place and is not complete yet. 
// Thus SSL_ERROR_WANT_WRITE means that the SSL state machine wants to perform a write
// and SSL_ERROR_WANT_READ means that the SSL state machine wants to perform a read. 
// We should wait untill the socket can perform these operations and then retry the said operation again
// with same parameters.
//
// It's also possible that with blocking sockets the code will block indefinitely in SSL_read.
// If we use select to check if the socket is readable it doesnt mean that SSL_read will return
// because the socket could be signalled readable but there's no application data arriving
// only SSL data. Thus SSL_read will block. 
//
// references:
// http://www.openssl.org/docs/ssl/SSL_read.html
// http://stackoverflow.com/questions/3952104/how-to-handle-openssl-ssl-error-want-read-want-write-on-non-blocking-sockets
// http://www.serverframework.com/asynchronousevents/2010/10/using-openssl-with-asynchronous-sockets.html
// http://www.mail-archive.com/openssl-users@openssl.org/msg34340.html

namespace corelib
{

sslsocket::sslsocket() : socket_(0), handle_(0), ssl_(nullptr), bio_(nullptr)
{}


sslsocket::sslsocket(native_socket_t sock, native_handle_t handle) : 
    socket_(sock), handle_(handle), ssl_(nullptr), bio_(nullptr)
{
    ssl_connect();
}

sslsocket::sslsocket(native_socket_t sock, native_handle_t handle, SSL* ssl, BIO* bio) : 
    socket_(sock), handle_(handle), ssl_(ssl), bio_(bio)
{}

sslsocket::sslsocket(sslsocket&& other) : 
    socket_(other.socket_), handle_(other.handle_), ssl_(other.ssl_), bio_(other.bio_)
{
    other.socket_ = 0;
    other.handle_ = 0;
    other.ssl_    = nullptr;
    other.bio_    = nullptr;
}

sslsocket::~sslsocket()
{
    close();
}

void sslsocket::begin_connect(ipv4addr_t host, port_t port)
{
    assert(!socket_);
    assert(!handle_);

    const auto& ret = begin_socket_connect(host, port);
    if (socket_)
        close();

    socket_ = ret.first;
    handle_ = ret.second;
}

void sslsocket::complete_connect()
{
    assert(socket_);

    const auto err = complete_socket_connect(handle_, socket_);
    if (err)
        throw tcp_exception("connect failed", err);

    ssl_connect();
}

void sslsocket::sendall(const void* buff, int len) 
{
    const char* ptr = static_cast<const char*>(buff);      

    int sent = 0;
    do 
    {
        sent += sendsome(ptr + sent, len - sent);
    }
    while (sent != len);
}

int sslsocket::sendsome(const void* buff, int len)
{
    // the SSL_read operation may fail because SSL handshake
    // is being done transparently and that requires IO on the socket
    // which cannot be completed at the time. This is indicated by 
    // SSL_ERROR_WANT_READ, SSL_ERROR_WANT_WRITE. When this happens
    // we need to wait utill the condition can be satisfied on the
    // underlying socket object (can read/write) and then restart
    // the SSL operation with the *same* parameters.        
    int sent = 0;
    do
    {
        const int ret = SSL_write(ssl_, buff, len);
        switch (SSL_get_error(ssl_, ret))
        {
            case SSL_ERROR_NONE:
                sent += ret;
                break;

            case SSL_ERROR_WANT_READ:
                ssl_wait_read();
                break;

            case SSL_ERROR_WANT_WRITE:
                ssl_wait_write();
                break;

            case SSL_ERROR_SYSCALL:
                {
                    const auto err = get_last_socket_error();
                    if (err != std::errc::operation_would_block)
                        throw socket::tcp_exception("socket send", err);

                    // on windows writeability is edge triggered, 
                    // i.e. the event is signaled once when the socket is writeable and a call
                    // to send clears the signal. the signal remains cleared
                    // untill send fails with WSAEWOULDBLOCK which will schedule
                    // the event for signaling once the socket can write more.        
                    return 0;
                }
                break;
            default:
               throw ssl_exception("SSL_write");
        }
    }
    while (!sent);

#if defined(WINDOWS_OS)
    // set the signal manually since the socket can write more,
    // so that we have the same semantics with linux.
    SetEvent(handle_);
#endif


    return sent;
}

int sslsocket::recvsome(void* buff, int capacity)
{
    // the SSL_read operation may fail because SSL handshake
    // is being done transparently and that requires IO on the socket
    // which cannot be completed at the time. This is indicated by 
    // SSL_ERROR_WANT_READ, SSL_ERROR_WANT_WRITE. When this happens
    // we need to wait utill the condition can be satisfied on the
    // underlying socket object (can read/write) and then restart
    // the SSL operation with the *same* parameters.        

    int recv = 0;
    do 
    {
        const int ret = SSL_read(ssl_, buff, capacity);
        switch (SSL_get_error(ssl_, ret))
        {
            case SSL_ERROR_NONE:
                recv += ret;
                break;

            case SSL_ERROR_WANT_WRITE:
                ssl_wait_write();
                break;

            case SSL_ERROR_WANT_READ:
                ssl_wait_read();
                break;

            case SSL_ERROR_SYSCALL:
               {
                    const auto err = get_last_socket_error();
                    if (err != std::errc::operation_would_block)
                        throw socket::tcp_exception("socket send", err);

                    return 0;
                }
                break;

            // socket was closed.
            case SSL_ERROR_ZERO_RETURN:
                return 0;                

            default:
                throw socket::ssl_exception("SSL_read");
        }
    }
    while (!recv);

    return recv;
}


void sslsocket::close()
{
    if (ssl_)
    {
        // SSL_free() also calls the free()ing procedures for indirectly 
        // affected items, if applicable: the buffering BIO, the read and
        // write BIOs, cipher lists specially created for this ssl, the
        // SSL_SESSION.
         
        SSL_free(ssl_);
        ssl_ = nullptr;
    }

    if (socket_)
    {
        closesocket(handle_, socket_);
        socket_ = 0;
        handle_ = 0;
    }
}

waithandle sslsocket::wait() const
{
    return waithandle {
        handle_, socket_, true, true
    };
}

waithandle sslsocket::wait(bool waitread, bool waitwrite) const
{
    assert(waitread || waitwrite);
    
    return waithandle {
        handle_, socket_, waitread, waitwrite
    };
}

sslsocket& sslsocket::operator=(sslsocket&& other)
{
    if (&other == this)
        return *this;

    sslsocket tmp(std::move(*this));

    std::swap(socket_, other.socket_);
    std::swap(handle_, other.handle_);
    std::swap(ssl_, other.ssl_);
    std::swap(bio_, other.bio_);
    return *this;
}

void sslsocket::ssl_wait_write()
{
    auto can_write = wait(false, true);
    corelib::wait_for(can_write);
}

void sslsocket::ssl_wait_read()
{
    auto can_read = wait(true, false);
    corelib::wait_for(can_read);
}

void sslsocket::ssl_connect()
{
    // create SSL and BIO objects and then initialize ssl client mode.
    // setup SSL session now that we have TCP connection.
    SSL_CTX* ctx = context_.ssl();

    ssl_ = SSL_new(ctx);
    if (!ssl_)
        throw socket::ssl_exception("SSL_new failed");

    // create new IO object
    bio_ = BIO_new_socket(socket_, BIO_NOCLOSE);
    if (!bio_)
        throw socket::ssl_exception("BIO_new_socket");

    // connect the IO object with SSL, this takes the ownership
    // of the BIO object.
    SSL_set_bio(ssl_, bio_, bio_);

    // go into client mode.
    while (true)
    {
        const int ret = SSL_connect(ssl_);
        if (ret == 1)
            break;

        const int err = SSL_get_error(ssl_, ret);
        switch (err)
        {
            case SSL_ERROR_WANT_READ:
                ssl_wait_read();
                break;

            case SSL_ERROR_WANT_WRITE:
                ssl_wait_write();
                break;

            default:
                throw socket::ssl_exception("SSL_connect");
        }
    }
}


} // corelib
