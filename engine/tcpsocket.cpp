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

#include "newsflash/config.h"

#include <algorithm> // for min/max
#include <cassert>

#include "sockets.h"
#include "tcpsocket.h"
#include "socketapi.h"
#include "platform.h"
#include "assert.h"

namespace newsflash
{

void TcpSocket::BeginConnect(ipv4addr_t host, ipv4port_t port)
{
    const auto& ret = begin_socket_connect(host, port);

    if (socket_)
        Close();

    socket_ = ret.first;
    handle_ = ret.second;
}

void TcpSocket::CompleteConnect(std::error_code* error)
{
    *error = complete_socket_connect(handle_, socket_);
}

void TcpSocket::SendAll(const void* buff, int len, std::error_code* error)
{
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
            if (err != std::errc::operation_would_block)
            {
                *error = err;
                return;
            }

            auto handle = GetWaitHandle(false, true);
            newsflash::wait(handle);
            ret = 0;
        }
        sent += ret;
    }
    while (sent < len);
}

int TcpSocket::SendSome(const void* buff, int len, std::error_code* error)
{
    assert(socket_);

    int flags = 0;

#if defined(LINUX_OS)
    flags = MSG_NOSIGNAL;

    // on linux the the packet is silently dropped if the device queue overflows (ENOBUFS)
    // so we check for the how much currently space is available
    // in the sendbuf and then crop the len if needed.
    // todo: should this be done for winsock also?
    int sendbuf = 1024;
    socklen_t optlen = sizeof(sendbuf);
    getsockopt(socket_, SOL_SOCKET, SO_SNDBUF, &sendbuf, &optlen);

    len = std::min(sendbuf, len);
#endif

    const char* ptr = static_cast<const char*>(buff);

    const int sent = ::send(socket_, ptr, len, flags);
    if (sent == OS_SOCKET_ERROR)
    {
        const auto err = get_last_socket_error();
        if (err != std::errc::operation_would_block)
            *error = err;

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


int TcpSocket::RecvSome(void* buff, int capacity, std::error_code* error)
{
    assert(socket_);

    char* ptr = static_cast<char*>(buff);

    const int ret = ::recv(socket_, ptr, capacity, 0);
    if (ret == OS_SOCKET_ERROR)
    {
        const auto err = get_last_socket_error();
        if (err != std::errc::operation_would_block)
            *error = err;

        return 0;
    }
    return ret;
}

void TcpSocket::Close()
{
    if (!socket_)
        return;

    closesocket(handle_, socket_);
    socket_ = 0;
    handle_ = 0;
}

waithandle TcpSocket::GetWaitHandle() const
{
    return { handle_, socket_, true, true };
}

waithandle TcpSocket::GetWaitHandle(bool waitread, bool waitwrite) const
{
    assert(waitread || waitwrite);

    return { handle_, socket_, waitread, waitwrite };
}

bool TcpSocket::CanRecv() const
{
    auto handle = GetWaitHandle(true, false);
    return wait_for(handle, std::chrono::milliseconds(0));

}

TcpSocket& TcpSocket::operator=(TcpSocket&& other)
{
    if (&other == this)
        return *this;

    TcpSocket tmp(std::move(*this));

    std::swap(socket_, other.socket_);
    std::swap(handle_, other.handle_);
    return *this;
}

} // newsflash



