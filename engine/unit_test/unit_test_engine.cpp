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
#include <newsflash/corelib/nntp/nntp.h>

//#include <boost/test/minimal.hpp>
#include <testlib/test_minimal.h>

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
#include "../account.h"
#include "../server.h"
#include "../file.h"

using namespace newsflash;

namespace unit_test {

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

class testcase 
{
public:
    ~testcase() = default;


};

class connection
{
public:
    connection(corelib::native_socket_t s) : socket_(s)
    {}

    connection(connection&& other) : socket_(other.socket_)
    {
        other.socket_ = 0;
    }

   ~connection()
    {
        if (socket_)
            corelib::closesocket(socket_);        
    }

    connection& operator=(connection&& other)
    {
        connection tmp(std::move(*this));
        socket_ = other.socket_;
        other.socket_= 0;
        return *this;
    }

    void greet()
    {
        send("200 Welcome\r\n");
        recv("CAPABILITIES\r\n");
        send("500 what?\r\n");
        recv("MODE READER\r\n");
        send("200 OK\r\n");
    }
private:
    void send(const std::string& str)
    {
        const char* ptr = str.data();
        std::size_t sent  = 0;
        do 
        {
            const int ret = ::send(socket_, ptr + sent, str.size() - sent, 0);
            TEST_REQUIRE(ret > 0);
            sent += ret;
        } 
        while (sent != str.size());
    }

    void recv(const std::string& str)
    {
        std::string buff;
        buff.resize(1024);
        const int ret = ::recv(socket_, &buff[0],  1024, 0);
        TEST_REQUIRE(ret > 0);
        TEST_REQUIRE(nntp::find_response(&buff[0], ret));
        TEST_REQUIRE(str == buff);
    }

private:
    corelib::native_socket_t socket_;
};

class server
{
public:
    server() : socket_(0)
    {}

   ~server()
    {
        corelib::closesocket(socket_);
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
        }
        TEST_REQUIRE(::listen(sock, 10) != corelib::OS_SOCKET_ERROR);
        TEST_REQUIRE(socket_ == 0);
        socket_ = sock;
        return port;
    }

    connection accept()
    {
        struct sockaddr_in addr {0};
        socklen_t len = sizeof(addr);
        corelib::native_socket_t sock = ::accept(socket_, static_cast<sockaddr*>((void*)&addr), &len);

        TEST_REQUIRE(sock != corelib::OS_INVALID_HANDLE);

        std::printf("Got new connection sock, %d\n", sock);

        return connection { sock };
    }

private:
    corelib::native_socket_t socket_;

};

} // unit_test

void test_download()
{
    unit_test::server server;
    server.open(4001);

    unit_test::tester listener;
    newsflash::engine engine(listener, "logs_test_download");    

    newsflash::account account;
    account.id                 = 123;
    account.name               = "test";
    account.username           = "foo";
    account.password           = "bar";
    account.secure             = newsflash::server { "localhost",  4001, false };
    account.max_connections    = 1;
    account.enable_compression = false;
    account.enable_pipelining  = false;
    account.fill               = false;

    newsflash::file file;
    file.articles = {"1", "2"};
    file.groups   = {"alt.binaries.foo", "alt.binaries.foo"};
    file.path     = "./files";
    file.name     = "test.file";
    file.desc     = "testing testing";
    file.size     = 1024;
    file.damaged  = false;
    file.account  = account.id;

    engine.download(account, file);
    engine.start();

    auto conn = server.accept();
    
    conn.greet();
    //conn.send_body("1", "alt.binaries.foo", )

}


int test_main(int, char*[])
{
    test_download();
    return 0;
}