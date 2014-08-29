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

#include <newsflash/config.h>

#include <boost/test/minimal.hpp>
#include <thread>
#include <deque>
#include <string>
#include "../connection.h"
#include "../action.h"
#include "../sockets.h"
#include "../socketapi.h"
#include "../logging.h"

namespace nf = newsflash;

#define REQUIRE_EXCEPTION(x) \
    try { \
        x; \
        BOOST_FAIL("exception was expected"); \
    } catch(const std::exception& e) {}

void server_loop(nf::native_socket_t l, std::deque<std::string> commands, std::deque<std::string> responses)
{
    struct sockaddr_in addr {0};
    socklen_t len = sizeof(addr);
    auto s = ::accept(l, static_cast<sockaddr*>((void*)&addr), &len);
    BOOST_REQUIRE(s != nf::OS_INVALID_SOCKET);


    const auto& greeting = responses.front() + "\r\n";
    int bytes = ::send(s, greeting.data(), greeting.size(), 0);
    BOOST_REQUIRE(bytes == (int)greeting.size());

    responses.pop_front();

    while (!commands.empty())
    {
        const auto& cmd = commands.front();
        const auto& ret = responses.front() + "\r\n";

        std::string buff;
        buff.resize(64);
        int bytes = ::recv(s, &buff[0], buff.size(), 0);
        if (bytes == -1)
            std::printf("error %s\n", strerror(errno));

        BOOST_REQUIRE(bytes > 2);

        buff.resize(bytes-2);
        std::printf("Received command: %s", buff.c_str());

        BOOST_REQUIRE(buff == cmd);

        bytes = ::send(s, ret.data(), ret.size(), 0);
        BOOST_REQUIRE(bytes == (int)ret.size());

        std::printf("Sending response: %s\n", ret.c_str());

        commands.pop_front();
        responses.pop_front();
    }

    nf::closesocket(s);
    nf::closesocket(l);
}


std::pair<nf::native_socket_t, int> open_server()
{
    auto p = 1119;
    auto s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    for (;;)
    {
        struct sockaddr_in addr {0};
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons(p);
        addr.sin_addr.s_addr = INADDR_ANY;

        const int ret = bind(s, static_cast<sockaddr*>((void*)&addr), sizeof(addr));
        if (ret != nf::OS_SOCKET_ERROR)
            break;

        std::cout << "bind failed: " << p;
        ++p;
    }
    BOOST_REQUIRE(listen(s, 10) != nf::OS_SOCKET_ERROR);

    //std::thread serv(std::bind(server_loop, s, std::move(commands), std::move(responses)));
    //serv.detach();

    return {s, p};
}

void test_connection()
{
    std::unique_ptr<nf::action> act;

    nf::connection conn;
    conn.on_action = [&](std::unique_ptr<nf::action> a) {
        act = std::move(a);
    };

    using state = nf::connection::state;

    BOOST_REQUIRE(conn.get_state() == state::disconnected);

    LOG_OPEN("test", "test_connection.log");

    // resolve fails
    // {
    //     nf::connection::server serv;
    //     serv.hostname = "foobar";
    //     serv.port     = 1818;
    //     conn.connect(serv);
    //     BOOST_REQUIRE(conn.get_state() == state::resolving);
    //     act->perform();
    //     BOOST_REQUIRE(act->has_exception());
    //     REQUIRE_EXCEPTION(conn.complete(std::move(act)));
    //     BOOST_REQUIRE(conn.get_state() == state::error);

    //     LOG_D("end");
    //     LOG_FLUSH();
    // }

    // // connect fails
    // {
    //     nf::connection::server serv;
    //     serv.hostname = "localhost";
    //     serv.port     = 1818;
    //     serv.use_ssl  = false;
    //     conn.connect(serv);
    //     act->perform();
    //     BOOST_REQUIRE(!act->has_exception());

    //     conn.complete(std::move(act));
    //     BOOST_REQUIRE(conn.get_state() == state::connecting);
    //     act->perform();
    //     BOOST_REQUIRE(act->has_exception());
    //     REQUIRE_EXCEPTION(conn.complete(std::move(act)));
    //     BOOST_REQUIRE(conn.get_state() == state::error);

    //     LOG_D("end");        
    //     LOG_FLUSH();        
    // }

    // // nntp init fails
    // {
    //     auto sp = open_server();

    //     nf::connection::server serv;
    //     serv.hostname = "localhost";
    //     serv.port     = sp.second;  //{}, {"this is total bullcrap"});
    //     serv.use_ssl  = false;
    //     conn.connect(serv);
    //     act->perform(); // resolve
    //     conn.complete(std::move(act));

    //     std::deque<std::string> commands;
    //     std::deque<std::string> responses = {
    //         "this is crap response"
    //     };
    //     std::thread thread(std::bind(server_loop, sp.first, commands, responses));

    //     std::this_thread::sleep_for(std::chrono::milliseconds(100));

    //     act->perform(); // connect
    //     conn.complete(std::move(act));
    //     BOOST_REQUIRE(conn.get_state() == state::initialize);
    //     act->perform(); // initialize
    //     BOOST_REQUIRE(act->has_exception());
    //     REQUIRE_EXCEPTION(conn.complete(std::move(act)));
    //     BOOST_REQUIRE(conn.get_state() == state::error);

    //     thread.join();

    //     LOG_D("end");        
    //     LOG_FLUSH();
    // }

    // authentication fails
    {

        auto sp = open_server();
        nf::connection::server serv;
        serv.hostname = "localhost";
        serv.port     = sp.second; 
        serv.use_ssl  = false;
        serv.username = "foo";
        serv.password = "bar";
        conn.connect(serv); 
        act->perform(); // resolve
        conn.complete(std::move(act));

        std::deque<std::string> commands = {
            "CAPABILITIES",
            "MODE READER",
            "AUTHINFO USER foo",
            "AUTHINFO PASS bar"
        };
        std::deque<std::string> responses = {
            "200 welcome",
            "500 what?",
            "480 authentication required",
            "381 password required",
            "482 authentication rejected"
        };
        std::thread thread(std::bind(server_loop, sp.first, commands, responses));

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        act->perform(); // connect
        conn.complete(std::move(act));
        act->perform(); // initialize
        REQUIRE_EXCEPTION(conn.complete(std::move(act)));
        BOOST_REQUIRE(conn.get_state() == state::error);

        thread.join();

        LOG_D("end");        
        LOG_FLUSH();
    }
}

int test_main(int argc, char* argv[])
{
    test_connection();

    return 0;
}
