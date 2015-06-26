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

//#include <boost/test/minimal.hpp>

#include <newsflash/config.h>

#include <newsflash/test_minimal.h>

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

native_socket_t openhost(int& port)
{
    auto sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    for (;;)
    {
        struct sockaddr_in addr {0};
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;

        const int ret = bind(sock, static_cast<sockaddr*>((void*)&addr), sizeof(addr));
        if (ret != OS_SOCKET_ERROR)
            break;

        TEST_MESSAGE("bind failed on port %d\n", port);        

        ++port;
    }

    TEST_REQUIRE(::listen(sock, 1) != OS_SOCKET_ERROR);

    return sock;
}

tcpsocket accept(native_socket_t fd)
{
    struct sockaddr_in addr {0};
    socklen_t len = sizeof(addr);
    auto sock = ::accept(fd, static_cast<sockaddr*>((void*)&addr), &len);
    TEST_REQUIRE(sock != OS_INVALID_SOCKET);

    auto handle = get_wait_handle(sock);
    return {sock, handle};
}

void test_connection_failure()
{
    TEST_BARK();

    // refused
    {
        std::uint32_t addr;
        resolve_host_ipv4("127.0.0.1", addr);

        tcpsocket sock;
        sock.begin_connect(addr, 8000);

        newsflash::wait(sock);
        REQUIRE_EXCEPTION(sock.complete_connect());
    }
}

void test_connection_success()
{
    TEST_BARK();

    int port = 8001;

    auto sock = openhost(port);

    std::uint32_t addr;
    resolve_host_ipv4("127.0.0.1", addr);

    tcpsocket tcp;
    tcp.begin_connect(addr, port);

    tcpsocket client = ::accept(sock);
    tcp.complete_connect();

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
        TEST_MESSAGE("Testing %d bytes", buff.len);

        std::memset(buff.buff, 0, buff.len);

        // send from host to client
        int sent = 0;
        int recv = 0;
        do 
        {
            if (sent != buff.len)
            {
                auto handle = client.wait(false, true);
                newsflash::wait(handle);
                TEST_REQUIRE(handle.write());                
            
                int ret = client.sendsome(buff.data + sent, buff.len - sent);
                sent += ret;

                TEST_MESSAGE("sent %d bytes", sent);
            }
            if (recv != buff.len)
            {
                auto handle = tcp.wait(true, false);
                newsflash::wait(handle);
                TEST_REQUIRE(handle.read());

                int ret = tcp.recvsome(buff.buff + recv, buff.len - recv);
                recv += ret;

                TEST_MESSAGE("recv %d bytes", recv);
            }
        } 
        while ((sent != buff.len) || (recv != buff.len));

        TEST_REQUIRE(!std::memcmp(buff.data, buff.buff, buff.len));

        TEST_MESSAGE("Passed %d byte buffer", buff.len);
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
    test_connection_failure();
    test_connection_success();

    return 0;
}
