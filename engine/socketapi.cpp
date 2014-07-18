// Copyright (c) 2010-2013 Sami Väisänen, Ensisoft 
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

#include <sstream>
#include <cassert>
#include <memory>
#include <stdexcept>
#include "sockets.h"
#include "socketapi.h"
#include "platform.h"
#include "utility.h"
#include "assert.h"

namespace {

#if defined(WINDOWS_OS)
    struct winsock {
        winsock()
        {
            WSADATA data;
            if (WSAStartup(MAKEWORD(1, 1), &data))
                throw std::runtime_error("winsock startup failed");
        }
       ~winsock()
        {
            WSACleanup();
        }
    } winsock_starter;
#endif

} // namespace

namespace corelib
{
#if defined(WINDOWS_OS)

ipv4addr_t resolve_host_ipv4(const std::string& hostname)
{
    // gethostbyname allocates data from TLS so it's thread safe
    const HOSTENT* hp = gethostbyname(hostname.c_str());
    if (hp == nullptr)
        return 0;

    const in_addr* addr = static_cast<const in_addr*>((void*)hp->h_addr);
    return ntohl(addr->s_addr);
}

std::pair<native_socket_t, native_handle_t> begin_socket_connect(ipv4addr_t host, port_t port)
{
    auto sock = make_unique_handle(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP), ::closesocket);
    if (sock.get() == INVALID_SOCKET)
        throw std::runtime_error("create socket failed");

    auto event = make_unique_handle(CreateEvent(NULL, FALSE, FALSE, NULL), CloseHandle);
    if (event.get() == NULL)
        throw std::runtime_error("create socket event failed");

    // NOTE: this makes the socket non-blocking
    if (WSAEventSelect(sock.get(), event.get(), FD_READ | FD_WRITE | FD_CONNECT) == SOCKET_ERROR)
        throw std::runtime_error("socket event select failed");

    struct sockaddr_in addr {0};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = htonl(host);
    if (connect(sock.get(), static_cast<sockaddr*>((void*)&addr), sizeof(sockaddr_in)))
    {
        const int err = WSAGetLastError();
        if (err != WSAEALREADY && err != WSAEWOULDBLOCK)
            throw std::runtime_error("connect failed");
    }

    SOCKET s = sock.release();
    HANDLE h = event.release();

    return {s, h};
}

std::error_code complete_socket_connect(native_handle_t handle, native_socket_t sock)
{
    int len = sizeof(len);
    int err = 0;
    const int ret = getsockopt(sock,
        SOL_SOCKET,
        SO_ERROR,
        static_cast<char*>((void*)&err),
        &len);
    if (ret == SOCKET_ERROR)
        throw std::runtime_error("getsockopt");

    return std::error_code(err, std::generic_category());
}

std::error_code get_last_socket_error()
{
    return std::error_code(WSAGetLastError(), 
        std::system_category());
}

native_handle_t get_wait_handle(native_socket_t sock)
{
    auto event = make_unique_handle(CreateEvent(NULL, FALSE, FALSE, NULL), CloseHandle);
    if (event.get() == NULL)
        throw std::runtime_error("create socket event failed");

    if (WSAEventSelect(sock, event.get(), FD_READ | FD_WRITE) == SOCKET_ERROR)
        throw std::runtime_error("socket event select failed");

    return event.release();

}

void closesocket(native_handle_t handle, native_socket_t sock)
{
    CHECK(WSAEventSelect(sock, NULL, 0), 0);
    CHECK(CloseHandle(handle), TRUE);
    CHECK(::closesocket(sock), 0);
}

void closesocket(native_socket_t sock)
{
    CHECK(::closesocket(sock), 0);
}



#elif defined(LINUX_OS)

ipv4addr_t resolve_host_ipv4(const std::string& hostname)
{
    struct addrinfo* addrs = nullptr;
    const int ret = getaddrinfo(hostname.c_str(), nullptr, nullptr, &addrs);
    if (ret)
        return 0;

    ipv4addr_t host = 0;
    // take the first IPv4 address
    const struct addrinfo* iter = addrs;
    while (iter)
    {
        if (iter->ai_addrlen == sizeof(sockaddr_in))
        {
            const sockaddr_in* ptr = static_cast<const sockaddr_in*>((void*)iter->ai_addr);
            host = ptr->sin_addr.s_addr;
            break;
        }
        iter = iter->ai_next;
    }
    freeaddrinfo(addrs);
    return ntohl(host);
}

std::pair<native_socket_t, native_handle_t> begin_socket_connect(ipv4addr_t host, port_t port)
{
    auto sock = make_unique_handle(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP), close);
    if (sock == -1)
        throw std::runtime_error("socket create failed");

    const int fd = sock.get();

    const int flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in addr {0};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = htonl(host);
    if (connect(fd, static_cast<sockaddr*>((void*)&addr), sizeof(sockaddr_in)))
    {
        const int err = errno;
        if (err != EINPROGRESS)
            throw std::runtime_error("connect failed");
    }

    sock.release();

    return {fd, fd};
}

std::error_code complete_socket_connect(native_handle_t handle, native_socket_t sock)
{
    assert(handle == sock);

    int err = 0;
    socklen_t len = sizeof(len);
    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &len))
        throw std::runtime_error("getsockopt failed");

    return std::error_code(err, std::generic_category());
}

std::error_code get_last_socket_error()
{
    return std::error_code(errno, std::generic_category());
}

void closesocket(native_handle_t handle, native_socket_t sock)
{
    CHECK(::close(sock), 0);
}

void closesocket(native_socket_t sock)
{
    CHECK(::close(sock), 0);
}

native_handle_t get_wait_handle(native_socket_t sock)
{
    return sock;
}


#endif

std::string format_ipv4(ipv4addr_t addr)
{
    std::stringstream ss;
    ss << ((addr >> 24) & 0xff) << "."
       << ((addr >> 16) & 0xff) << "."
       << ((addr >> 8) & 0xff) << "."
       << (addr & 0xff);

    std::string s;
    ss >> s;
    return s;
}

} // corelib
