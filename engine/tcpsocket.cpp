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

#include <cassert>
#include "tcpsocket.h"
#include "socketapi.h"
#include "platform.h"
#include "assert.h"

namespace newsflash
{

tcpsocket::tcpsocket() : socket_(0), handle_(0), state_(state::nothing)
{}

tcpsocket::tcpsocket(native_socket_t sock, native_handle_t handle) : 
    socket_(sock), handle_(handle), state_(state::ready)
{}

tcpsocket::tcpsocket(tcpsocket&& other) : 
    socket_(other.socket_), handle_(other.handle_), state_(other.state_)
{
    other.socket_ = 0;
    other.handle_ = 0;
    other.state_  = state::nothing;
}

tcpsocket::~tcpsocket()
{
    close();
}

void tcpsocket::connect(ipv4_addr_t host, uint16_t port) 
{
    assert(state_ == state::nothing && "Socket is already connected");
    assert(!socket_ && !handle_);

    const auto& ret = begin_socket_connect(host, port);

    if (socket_)
        close();

    socket_ = ret.first;
    handle_ = ret.second;    
    state_  = state::connecting;
}

native_errcode_t tcpsocket::complete_connect()
{
    assert(state_ == state::connecting && "Socket is not in connecting state");
    assert(socket_ && handle_);

    state_ = state::ready;

    const native_errcode_t err = complete_socket_connect(handle_, socket_);
    if (err)
        close();
    
    return err;
}

void tcpsocket::sendall(const void* buff, int len)
{
    assert(state_ == state::ready && "Socket is not connected");
    assert(socket_);

    int flags = 0;

#if defined(LINUX_OS)
    // SIGPIPE is generated when trying to send over a socket that
    // has been closed by either party. we dont need this signal
    flags = MSG_NOSIGNAL;
#endif

    const char* ptr = static_cast<const char*>(buff);
    int sent = 0;
    do 
    {
        int ret = ::send(socket_, ptr + sent, len - sent, flags);
        if (ret == OS_SOCKET_ERROR)
        { 
            const auto err = get_last_socket_error();
            if (err != OS_ERR_WOULD_BLOCK && err != OS_ERR_AGAIN)
                throw socket::io_exception("socket send", get_last_socket_error());

            auto handle = wait(false, true);
            newsflash::wait(handle);
            assert(handle.write());

            ret = 0;
        }
        sent += ret;
    }
    while (sent < len);
}

int tcpsocket::sendsome(const void* buff, int len)
{
    assert(state_ == state::ready && "socket is not connected");
    assert(socket_);

    int flags = 0;

#if defined(LINUX_OS)
    flags = MSG_NOSIGNAL;
#endif
    
    const char* ptr = static_cast<const char*>(buff);

    const int sent = ::send(socket_, ptr, len, flags);
    if (sent == OS_SOCKET_ERROR)
    {
        const auto err = get_last_socket_error();
        if (err == OS_ERR_WOULD_BLOCK && err != OS_ERR_AGAIN)
            throw socket::io_exception("socket send", get_last_socket_error());

        // on windows writeability is edge triggered, 
        // i.e. the event is signaled once when the socket is writeable and a call
        // to send clears the signal. the signal remains cleared
        // untill send fails with WSAEWOULDBLOCK which will schedule
        // the event for signaling once the socket can write more.        
        return 0;
    }
#if defined(WINDOWS_OS)
    // set the signal manually since the socket can write more,
    // so that we have the same semantics with linux.
    CHECK(SetEvent(handle_), TRUE);
#endif

    return sent;
}


int tcpsocket::recvsome(void* buff, int capacity)
{
    assert(state_ == state::ready && "Socket is not connected");
    assert(socket_ && handle_);

    char* ptr = static_cast<char*>(buff);

    const int ret = ::recv(socket_, ptr, capacity, 0);
    if (ret == OS_SOCKET_ERROR)
    {
        const auto err = get_last_socket_error();
        if (err != OS_ERR_WOULD_BLOCK && err != OS_ERR_AGAIN)
            throw socket::io_exception("socket recv", err);

        return 0;
    }
    return ret;
}

void tcpsocket::close()
{
    if (!socket_)
        return;

    closesocket(handle_, socket_);
    socket_ = 0;
    handle_ = 0;
    state_  = state::nothing;
}

waithandle tcpsocket::wait() const
{
    return waithandle {
        handle_, socket_, true, true
    };
}

waithandle tcpsocket::wait(bool waitread, bool waitwrite) const
{
    assert(waitread || waitwrite);
    
    return waithandle {
        handle_, socket_, waitread, waitwrite
    };
}

tcpsocket& tcpsocket::operator=(tcpsocket&& other)
{
    if (&other == this)
        return *this;

    tcpsocket tmp(std::move(*this));

    std::swap(socket_, other.socket_);
    std::swap(handle_, other.handle_);
    std::swap(state_, other.state_);
    return *this;
}

} // newsflash



