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
#include <boost/lexical_cast.hpp>

#include "../sockets.h"
#include "../connection.h"
#include "../tcpsocket.h"
#include "../sslsocket.h"
#include "../waithandle.h"
#include "../buffer.h"
#include "../cmdlist.h"
#include "../protocol.h"

using namespace newsflash;

struct tester {
    std::mutex mutex;
    std::condition_variable errcond;
    std::condition_variable readycond;
    std::string user;
    std::string pass;

    tester() : error(connection::error::none)
    {}

    tester(newsflash::connection& conn) : error(connection::error::none)
    {
        conn.on_error = std::bind(&tester::on_error, this,
            std::placeholders::_1);
        conn.on_auth = std::bind(&tester::on_auth, this,
            std::placeholders::_1, std::placeholders::_2);
    }

    void on_auth(std::string& username, std::string& password)
    {
        username = user;
        password = pass;
    }

    void on_error(connection::error e)
    {
        std::lock_guard<std::mutex> lock(mutex);
        error = e;
        errcond.notify_one();
    }

    void on_ready()
    {
        std::lock_guard<std::mutex> lock(mutex);
        ready = true;
        readycond.notify_one();
    }

    void wait_error()
    {
        std::unique_lock<std::mutex> lock(mutex);
        bool ret = errcond.wait_for(lock, std::chrono::seconds(50),
            [&]() { return error != connection::error::none; });

        if (!ret)
            throw std::runtime_error("WAIT FAILED");
    }

    void wait_ready()
    {
        std::unique_lock<std::mutex> lock(mutex);
        bool ret = readycond.wait_for(lock, std::chrono::seconds(50),
            [&]() { return ready; });

        if (!ret)
            throw std::runtime_error("WAIT FAILED");

        ready = false;
    }

    connection::error error;
    bool ready;
};

class server
{
public:    
   ~server()
    {
        closesocket(listener_);
        closesocket(socket_);
    }

    std::uint16_t open(std::uint16_t port)
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

            std::cout << "bind failed: " << port;
            //std::this_thread::sleep_for(std::chrono::seconds(1));
            ++port;
        }

        BOOST_REQUIRE(listen(sock, 10) != OS_SOCKET_ERROR);
        listener_ = sock;        
        return port;
    }

    void accept()
    {
        struct sockaddr_in addr {0};
        socklen_t len = sizeof(addr);
        socket_ = ::accept(listener_, static_cast<sockaddr*>((void*)&addr), &len);
        BOOST_REQUIRE(socket_ != OS_INVALID_SOCKET);
    }

    void send(const std::string& str)
    {
        std::string nntp(str);
        nntp.append("\r\n");

        const int ret = ::send(socket_, &nntp[0], nntp.size(), 0);
        BOOST_REQUIRE(ret == (int)nntp.size());
    }

    void recv(const std::string& str)
    {
        std::string s;
        s.resize(100);
        const int ret = ::recv(socket_, &s[0], s.size(), 0);
        BOOST_REQUIRE(ret > 2);
        s.resize(ret-2); // omit \r\n
        //std::cout << s;
        BOOST_REQUIRE(s == str);
    }

    void recv(std::string& str)
    {
        str.resize(100);
        const int ret = ::recv(socket_, &str[0], str.size(), 0);
        BOOST_REQUIRE(ret != -1);
        str.resize(ret);
    }

private:
    native_socket_t listener_;
    native_socket_t socket_;
};

void unit_test_connection_resolve_error(bool ssl)
{
    tester test;

    newsflash::connection conn("clog");
    conn.on_error = std::bind(&tester::on_error, &test, 
        std::placeholders::_1);    
    conn.connect("asgasgasg", 8888, ssl);

    test.wait_error();
    BOOST_REQUIRE(test.error == connection::error::resolve);
}

void unit_test_connection_refused(bool ssl)
{
    tester test;

    newsflash::connection conn("clog");
    conn.on_error = std::bind(&tester::on_error, &test, 
        std::placeholders::_1);    
    conn.connect("localhost", 2119, ssl);

    test.wait_error();
    BOOST_REQUIRE(test.error == connection::error::refused);
}

void unit_test_connection_interrupted(bool ssl)
{
    tester test;

    newsflash::connection conn("clog");
    conn.on_error = std::bind(&tester::on_error, &test, 
        std::placeholders::_1);
    conn.connect("news.budgetnews.net", 119, false);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void unit_test_connection_forbidden(bool ssl)
{
    tester test;
    test.user = "foobar";
    test.pass = "foobar";

    newsflash::connection conn("clog");
    conn.on_error = std::bind(&tester::on_error, &test,
        std::placeholders::_1);
    conn.on_auth = std::bind(&tester::on_auth, &test,
        std::placeholders::_1, std::placeholders::_2);
    conn.connect("news.astraweb.com", 1818, false);

    test.wait_error();
    BOOST_REQUIRE(test.error == connection::error::forbidden);
}

void unit_test_connection_timeout_error(bool ssl)
{
    tester test;

    newsflash::connection conn("clog");
    conn.on_error = std::bind(&tester::on_error, &test,
        std::placeholders::_1);        
    conn.connect("www.google.com", 80, false);

    test.wait_error();
    BOOST_REQUIRE(test.error == connection::error::timeout);
}

void unit_test_connection_protocol_error(bool ssl)
{
    // todo:
}

void unit_test_connection_success(bool ssl)
{
    tester test;

    newsflash::connection conn("clog");
    conn.on_error = std::bind(&tester::on_error, &test, 
        std::placeholders::_1);
    conn.on_ready = std::bind(&tester::on_ready, &test);
    conn.connect("freenews.netfront.net", 119, false);

    test.wait_ready();
    BOOST_REQUIRE(test.error == connection::error::none);
}

void unit_test_cmdlist()
{
    tester test;
    server serv;

    auto port = serv.open(8181);

    newsflash::connection conn("clog");
    conn.on_error = std::bind(&tester::on_error, &test,
        std::placeholders::_1);
    conn.on_ready = std::bind(&tester::on_ready, &test);
    conn.connect("localhost", port, false);

    serv.accept();
    serv.send("200 Welcome posting allowed");
    serv.recv("CAPABILITIES");
    serv.send("500 what?");
    serv.recv("MODE READER");
    serv.send("200 posting allowed");

    test.wait_ready();

    // run to completion    
    {
        class testlist : public newsflash::cmdlist
        {
        public:
            testlist() : counter_(0)
            {}

            bool run(protocol& proto) override
            {
                const auto& article = boost::lexical_cast<std::string>(counter_);
                newsflash::buffer buff;
                buff.reserve(100);

                proto.body(article, buff);

                ++counter_;

                return bool(counter_ != 10);
            }
        private:
            int counter_;
        };

        auto list = std::make_shared<testlist>();

        conn.execute(list);

        for (int i=0; i<10; ++i)
        {
            std::string str;
            std::stringstream ss;
            ss << "BODY " << i;
            serv.recv(ss.str());
            serv.send("420 no article with that message id");
        }

        test.wait_ready();
    }

    // test cancellation
    {
        class testlist : public newsflash::cmdlist
        {
        public:
            testlist()
            {}

            bool run(protocol& proto) override
            {
                newsflash::buffer buff;
                buff.reserve(1024);

                proto.body("1234", buff);
                return true;
            }
        };

        auto list = std::make_shared<testlist>();

        conn.execute(list);
        conn.cancel();
        serv.recv("BODY 1234");
        serv.send("420 no article with that message id");

        test.wait_ready();
    }
}


int test_main(int argc, char* argv[])
{
    unit_test_connection_resolve_error(false);
    unit_test_connection_refused(false);
    unit_test_connection_interrupted(false);
    unit_test_connection_forbidden(false);
    unit_test_connection_timeout_error(false);
    unit_test_connection_success(false);
    unit_test_cmdlist();
    return 0;
}
