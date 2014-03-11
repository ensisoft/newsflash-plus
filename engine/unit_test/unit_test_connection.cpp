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
#include "../connection.h"
#include "../tcpsocket.h"
#include "../sslsocket.h"
#include "../command.h"
#include "../waithandle.h"
#include "../buffer.h"

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
        bool ret = errcond.wait_for(lock, std::chrono::seconds(500000),
            [&]() { return error != connection::error::none; });

        if (!ret)
            throw std::runtime_error("WAIT FAILED");
    }

    void wait_ready()
    {
        std::unique_lock<std::mutex> lock(mutex);
        bool ret = readycond.wait_for(lock, std::chrono::seconds(500000),
            [&]() { return ready; });

        if (!ret)
            throw std::runtime_error("WAIT FAILED");

        ready = false;
    }

    connection::error error;
    bool ready;
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
    // todo:
}


int test_main(int argc, char* argv[])
{
    // unit_test_connection_resolve_error(false);
    // unit_test_connection_refused(false);
    // unit_test_connection_interrupted(false);
    unit_test_connection_forbidden(false);
    // unit_test_connection_timeout_error(false);
    //unit_test_connection_success(false);
    unit_test_cmdlist();
    return 0;
}