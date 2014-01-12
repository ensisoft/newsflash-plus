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

#include <boost/test/minimal.hpp>
#include <thread>
#include <chrono>
#include "../socketapi.h"
#include "../tcpsocket.h"
#include "../types.h"
#include "../platform.h"
#include "unit_test_common.h"

#if defined(LINUX_OS)
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <arpa/inet.h>
#  include <netinet/in.h>
#endif


using namespace newsflash;

native_socket_t openhost(int port)
{
    auto sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in addr {0};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    while (::bind(sock, static_cast<sockaddr*>((void*)&addr), sizeof(addr)) == OS_SOCKET_ERROR)
        std::this_thread::sleep_for(std::chrono::seconds(1));

    //BOOST_REQUIRE(::bind(sock, static_cast<sockaddr*>((void*)&addr), sizeof(addr)) != OS_SOCKET_ERROR);
    BOOST_REQUIRE(::listen(sock, 1) != OS_SOCKET_ERROR);

    return sock;
}

tcpsocket accept(native_socket_t fd)
{
    struct sockaddr_in addr {0};
    socklen_t len = sizeof(addr);
    auto sock = ::accept(fd, static_cast<sockaddr*>((void*)&addr), &len);
    BOOST_REQUIRE(sock != OS_INVALID_SOCKET);

    auto handle = get_wait_handle(sock);
    return {sock, handle};
}

void test_connection_refused()
{
    {
        tcpsocket tcp;
        tcp.connect(resolve_host_ipv4("127.0.0.1"), 8000);

        auto handle = tcp.wait();
        wait(handle);
        BOOST_REQUIRE(handle.read());
        BOOST_REQUIRE(tcp.complete_connect() != 0);
    }

    {
        tcpsocket tcp;
        tcp.connect(resolve_host_ipv4("192.168.0.240"), 9999);

        auto handle = tcp.wait();
        wait(handle);
        BOOST_REQUIRE(handle.read());
        BOOST_REQUIRE(tcp.complete_connect() != 0);
    }

    {
        tcpsocket tcp;
        tcp.connect(resolve_host_ipv4("192.168.0.1"), 9999);

        auto handle = tcp.wait();
        wait(handle);
        BOOST_REQUIRE(handle.read());
        BOOST_REQUIRE(tcp.complete_connect() != 0);
    }
}

void test_connection_success()
{
    auto sock = openhost(8001);

    tcpsocket tcp;
    tcp.connect(resolve_host_ipv4("127.0.0.1"), 8001);

    tcpsocket client = ::accept(sock);
    BOOST_REQUIRE(tcp.complete_connect() == 0);

    newsflash::closesocket(sock);

    // allocate buffers of various sizes and transfer them
    struct buffer {
        int   len;        
        char* data;
        char* buff;
    } buffers[] = {
        {10},
        {1024},
        {1024 * 1024},
        {1024 * 1024 * 5}
    };

    for (auto& buff : buffers)
    {
        buff.data = new char[buff.len];
        buff.buff = new char[buff.len];
        fill_random(buff.data, buff.len);
    }

    for (auto& buff : buffers)
    {
        std::memset(buff.buff, 0, buff.len);

        // send from host to client
        int sent = 0;
        int recv = 0;
        do 
        {
            if (sent != buff.len)
            {
                auto handle = client.wait(false, true);
                wait(handle);
                BOOST_REQUIRE(handle.write());                
            
                int ret = client.sendsome(buff.data + sent, buff.len - sent);
                sent += ret;
            }
            if (recv != buff.len)
            {
                auto handle = tcp.wait(true, false);
                wait(handle);
                BOOST_REQUIRE(handle.read());

                int ret = tcp.recvsome(buff.buff + recv, buff.len - recv);
                recv += ret;
            }
        } 
        while ((sent != buff.len) || (recv != buff.len));

        BOOST_REQUIRE(!std::memcmp(buff.data, buff.buff, buff.len));
    }

    for (auto& buff : buffers)
    {        
        delete [] buff.data;
        delete [] buff.buff;
    }

    newsflash::closesocket(sock);
}

int test_main(int, char*[])
{
    test_connection_refused();
    test_connection_success();

    return 0;
}
