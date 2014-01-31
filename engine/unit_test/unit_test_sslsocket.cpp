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
#include "../sslsocket.h"
#include "../platform.h"
#include "../types.h"
#include "unit_test_common.h"

#if defined (LINUX_OS)
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

    BOOST_REQUIRE(listen(sock, 1) != OS_SOCKET_ERROR);

    return sock;
}

sslsocket accept(native_socket_t fd)
{
    
}

void test_connection_refused()
{

}

void test_connection_success()
{}

int test_main(int argc, char* argv[])
{
    test_connection_refused();
    test_connection_success();

    return 0;
}