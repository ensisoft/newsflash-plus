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

namespace {

class sslcontext 
{
public:
    static sslcontext& get()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!instance_)
            instance_ = new sslcontext();

        return *instance_;
    }
    void addref()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        ++refcount_;
    }
    void decref()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (--refcount_ == 0)
        {
            delete instance_;
            instance_ = nullptr;
        }
    }

    SSL_CTX* ssl()
    {
        return ssl_ctx_;
    }
private:
    sslcontext() : ssl_method_(nullptr), ssl_ctx_(nullptr), locks_(nullptr), refcount_(0)
    {
        const int locks = CRYPTO_num_locks();
        locks_ = new std::mutex[locks];

        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_ssl_algorithms();

        ssl_method_ = const_cast<SSL_METHOD*>(SSLv23_method());
        if (!ssl_method_)
            throw std::runtime_error("SSLv23_method");
        ssl_ctx_ = SSL_CTX_new(ssl_method_);
        if (!ssl_ctx_)
            throw std::runtime_error("SSL_CTX_new");


        // todo: should load the trusted Certifying Authorities list/file

        // these callback functions need to be set last because
        // the setup functions above call these (especially the locking function)
        // which calls back to us and whole thing goes astray. 
        // Calling the functions above without locking should be safe
        // considering that this constructor code is already protected from MT access
        // If there are random crashes, maybe this assumption is wrong
        // and locking needs to be taken care of already.
        CRYPTO_set_id_callback(thread_id_callback);
        CRYPTO_set_locking_callback(locking_callback);
    }

   ~sslcontext()
    {
        SSL_CTX_free(ssl_ctx_);

        delete [] locks_;
    }

    static unsigned long thread_id_callback()
    {
    // std::thread_id: has no conversion to unsigned long
    // so must fallback on native funcs
    #if defined(WINDOWS_OS)
        return GetCurrentThreadId();
    #elif defined(LINUX_OS)
        return pthread_self();
    #endif
    }

    static void locking_callback(int mode, int index, const char*, int)
    {
        sslcontext& ctx = sslcontext::get();

        if (mode & CRYPTO_LOCK)
            ctx.locks_[index].lock();
        else
            ctx.locks_[index].unlock();
    }

    static std::mutex mutex_;
    static sslcontext* instance_;

    SSL_METHOD* ssl_method_;
    SSL_CTX*    ssl_ctx_;
    std::mutex* locks_;
    int refcount_;
};

    std::mutex sslcontext::mutex_;
    sslcontext* sslcontext::instance_;

} // namespace

namespace newsflash
{

sslsocket::sslsocket() : socket_(0), handle_(0), state_(state::nothing), 
    ssl_(nullptr), bio_(nullptr)
{
    sslcontext& ctx = sslcontext::get();
    ctx.addref();
}

sslsocket::~sslsocket()
{
    sslcontext& ctx = sslcontext::get();
    ctx.decref();

    close();
}

void sslsocket::connect(ipv4addr_t host, port_t port)
{
    assert(!socket_);
    assert(!handle_);

    const auto& ret = begin_socket_connect(host, port);
    if (socket_)
        close();

    socket_ = ret.first;
    handle_ = ret.second;
    state_  = state::connecting;
}

native_errcode_t sslsocket::complete_connect()
{
    assert(state_ == state::connecting && "Socket is not in connecting state");
    assert(socket_);
    assert(handle_);

    const native_errcode_t err = complete_socket_connect(handle_, socket_);
    if (err)
    {
        close();
        return err;
    }
    // setup SSL session now that we have TCP connection.
    sslcontext& ctx = sslcontext::get();

    ssl_ = SSL_new(ctx.ssl());
    if (!ssl_)
        throw std::runtime_error("SSL_new failed");

    // create new IO object
    bio_ = BIO_new_socket(socket_, BIO_NOCLOSE);
    if (!bio_)
        throw std::runtime_error("BIO_new_socket");

    SSL_set_bio(ssl_, bio_, bio_);

    int error = SSL_connect(ssl_);
    switch (SSL_get_error(ssl_,error)) 
    {
        case SSL_ERROR_NONE:
            break;
        // SSL_ERROR_SYSCALL
        // SSL_ERROR_SSL
        default:
            throw std::runtime_error("SSL_connect");
    }
    return 0;
}

void sslsocket::sendall(const void* buff, int len) 
{
    int sent = 0;
    do 
    {
        // the SSL_write operation may fail because of SSL handshake
        // is being done transparently. In this case SSL_ERROR_WANT_WRITE
        // is reported and we must try sending data again        
        const int ret = SSL_write(ssl_, buff, len);
        if (ret <= 0)
        {
            switch (SSL_get_error(ssl_, ret))
            {
                case SSL_ERROR_SYSCALL:
                    throw std::runtime_error("socket send");

            }
        }
        sent += ret;
    }
    while (sent < len);
}

int sslsocket::sendsome(const void* buff, int len)
{
    return 0;
}

int sslsocket::recvsome(void* buff, int capacity)
{
    // the SSL_read operation may fail because SSL handshake
    // is being done transparently and we have to restart the
    // same IO operation. This is indicated by SSL_ERROR_WANT_READ
    int ret = 0;

    while (true)
    {
        ret = SSL_read(ssl_, buff, capacity);
        if (ret > 0)
            break;
        switch (SSL_get_error(ssl_, ret))
        {
            case SSL_ERROR_SYSCALL:
                if (ret == 0)
                    return 0;
                break;
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_READ:
            default:
                throw std::runtime_error("SSL_read");
        }
    }
    return ret;
}


void sslsocket::close()
{
    if (ssl_)
    {
        SSL_shutdown(ssl_);
        SSL_free(ssl_);
        ssl_ = nullptr;
        bio_ = nullptr;
    }
    if (socket_)
    {
        closesocket(socket_, handle_);
        socket_ = 0;
        handle_ = 0;
    }
    state_ = state::nothing;
}

waithandle sslsocket::wait() const
{
    return waithandle {
        handle_, waithandle::type::socket, true, true
    };
}

waithandle sslsocket::wait(bool waitread, bool waitwrite) const
{
    assert(waitread || waitwrite);
    
    return waithandle {
        handle_, waithandle::type::socket, waitread, waitwrite
    };
}


} // newsflash
