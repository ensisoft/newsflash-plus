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

#include <newsflash/workaround.h>

#include <boost/test/minimal.hpp>

#include <corelib/socketapi.h>
#include <corelib/sockets.h>
#include <corelib/socket.h>
#include <corelib/filesys.h>
#include <condition_variable>
#include <mutex>
#include <cstdio>
#include "../engine.h"
#include "../connection.h"
#include "../task.h"
#include "../listener.h"

using namespace newsflash;

class tester : public listener 
{
public:
    tester() : notify_count_(0)
    {}

    virtual void handle(const newsflash::error& error) override
    {

    }

    virtual void acknowledge(const newsflash::file& file) override
    {

    }

    virtual void notify()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++notify_count_;
        cond_.notify_one();
    }

    void wait_notify()
    {
        while (true)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (!notify_count_)
            {
                cond_.wait(lock);
                if (!notify_count_)
                    continue;
            }
            notify_count_--;
            break;
        }
    }

private:
    std::mutex mutex_;
    std::condition_variable cond_;
    std::size_t notify_count_;
};


class server
{
public:
    server() : socket_(0), client_(0)
    {}

   ~server()
    {
        corelib::closesocket(socket_);
        corelib::closesocket(client_);
    }

    std::uint16_t open(std::uint16_t port)
    {
        auto sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        for (;;)
        {
            struct sockaddr_in addr {0};
            addr.sin_family = AF_INET;
            addr.sin_port   = htons(port);
            addr.sin_addr.s_addr = INADDR_ANY;

            const int ret = ::bind(sock, static_cast<sockaddr*>((void*)&addr), sizeof(addr));
            if (ret != corelib::OS_SOCKET_ERROR)
                break;

            std::printf("bind failed %d\n", port);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            ++port;
        }
        BOOST_REQUIRE(::listen(sock, 10) != corelib::OS_SOCKET_ERROR);
        BOOST_REQUIRE(socket_ == 0);
        socket_ = sock;
        return port;
    }

private:
    corelib::native_socket_t socket_;
    corelib::native_socket_t client_;
};

void test_download()
{
    server serv;    
    tester list;
    engine eng(list, "logs_test_download");    


}


int test_main(int, char*[])
{
    test_download();
    return 0;
}