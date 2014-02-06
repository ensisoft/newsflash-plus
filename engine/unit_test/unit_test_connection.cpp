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

void unit_test_connection_resolve_error(bool ssl)
{
    cmdqueue in, out;

    connection::server host;
    host.addr = "asdgljasljgas";
    host.port = 8888;
    host.ssl  = ssl;

    connection conn("clog", host, in, out);
    wait(conn);

    auto status = conn.get_status();
    BOOST_REQUIRE(status.state == connection::state::error);
    BOOST_REQUIRE(status.error == connection::error::resolve);
}

void unit_test_connection_refused(bool ssl)
{
    cmdqueue in, out;

    connection::server host;
    host.addr = "localhost";
    host.port = 2119;
    host.ssl  = ssl;

    connection conn("clog", host, in, out);
    wait(conn);

    auto status = conn.get_status();
    BOOST_REQUIRE(status.state == connection::state::error);
    BOOST_REQUIRE(status.error == connection::error::refused);
}

void unit_test_connection_interrupted(bool ssl)
{
    cmdqueue in, out;

    connection::server host;
    host.addr = "news.budgetnews.net";
    host.port = 199;
    host.ssl  = false;

    connection* conn = new connection("clog", host, in, out);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    delete conn;
}

void unit_test_connection_forbidden(bool ssl)
{
    cmdqueue in, out;

    connection::server host;
    host.addr = "news.budgetnews.net";
    host.port = 119;
    host.ssl  = false;
    host.pass = "foobar";
    host.user = "foobar";

    connection conn("clog", host, in, out);
    wait(conn);

    auto status = conn.get_status();
    BOOST_REQUIRE(status.state == connection::state::error);
    BOOST_REQUIRE(status.error == connection::error::forbidden);
}

void unit_test_connection_timeout_error(bool ssl)
{
    cmdqueue in, out;

    connection::server host;
    host.addr = "www.google.com";
    host.port = 80;
    host.ssl  = false;

    connection conn("clog", host, in, out);
    wait(conn);

    auto status = conn.get_status();
    BOOST_REQUIRE(status.state == connection::state::error);
    BOOST_REQUIRE(status.error == connection::error::timeout);
}

void unit_test_connection_protocol_error(bool ssl)
{
    // todo:
}

void unit_test_connection_success(bool ssl)
{
    cmdqueue commands, responses;

    connection::server host;
    host.addr = "freenews.netfront.net";
    host.port = 119;
    host.ssl  = ssl;

    connection conn("clog", host, commands, responses);
    wait(conn);

    auto status = conn.get_status();
    BOOST_REQUIRE(status.state == connection::state::idle);
    BOOST_REQUIRE(status.error == connection::error::none);

    {
        commands.push_back(new cmd_list(0, 0));

        wait(responses);
        auto ret = responses.try_get_front();

        auto& list = command_cast<cmd_list>(ret);
        BOOST_REQUIRE(!list.empty());
    }

    // // no such group
    {
        commands.push_back(new cmd_group(0, 0, "mozilla.support.thunderbird.blala"));

        wait(responses);
        auto ret = responses.get_front();

        auto& group = command_cast<cmd_group>(ret);

        // BOOST_REQUIRE(group.id == cmd.id);
        // BOOST_REQUIRE(group.taskid == cmd.taskid);
        BOOST_REQUIRE(group.success() == false);
        BOOST_REQUIRE(group.count() == 0);
        BOOST_REQUIRE(group.low() == 0);
        BOOST_REQUIRE(group.high() == 0);
    }

    // a valid group
    {
        commands.push_back(new cmd_group(0, 0, "mozilla.support.thunderbird"));

        wait(responses);
        auto ret = responses.get_front();

        auto& group = command_cast<cmd_group>(ret);

        BOOST_REQUIRE(group.success());
        BOOST_REQUIRE(group.count());
        BOOST_REQUIRE(group.high());
        BOOST_REQUIRE(group.low());
    }

    // no such group for xover
    {
        commands.push_back(new cmd_xover(0, 0, "mozilla.support.thunderbird.blala", 0, 1));

        wait(responses);
        auto ret = responses.get_front();

        auto& xover = command_cast<cmd_xover>(ret);
        BOOST_REQUIRE(xover.success() == false);
    }

    // succesful xover
    {
        commands.push_back(new cmd_xover(1, 1, "mozilla.support.thunderbird", 0, 1000));

        wait(responses);
        auto ret = responses.get_front();

        auto& xover = command_cast<cmd_xover>(ret);
        BOOST_REQUIRE(xover.success() == true);
        BOOST_REQUIRE(xover.empty() == false);
    }


    // test body.
    {

        std::string available;
        std::string unavailable;

        // request some group information and try to find
        // article ids for available and unavailable messages.
        {

            commands.push_back(new cmd_group(1, 1, "mozilla.support.thunderbird"));
            auto response = responses.get_front();
            auto group = command_cast<cmd_group>(response);

            available   = boost::lexical_cast<std::string>(group.high());
            unavailable = boost::lexical_cast<std::string>(group.high() + 1000);
        }

        // unavailable
        {
            commands.push_back(new cmd_body(1, 1, unavailable, {"mozilla.support.thunderbird"}));

            auto response = responses.get_front();
            auto body = command_cast<cmd_body>(response);
            BOOST_REQUIRE(body.status() == cmd_body::cmdstatus::unavailable);
            BOOST_REQUIRE(body.empty());
        }

        // available
        {
            commands.push_back(new cmd_body(1, 1, available, {"mozilla.support.thunderbird"}));

            auto response = responses.get_front();
            auto body = command_cast<cmd_body>(response);

            BOOST_REQUIRE(body.status() == cmd_body::cmdstatus::success);
            BOOST_REQUIRE(body.empty() == false);
            
        }
    }

}

void unit_test_multiple_connections(bool ssl)
{
    cmdqueue commands, responses;

    connection::server host;
    host.addr = "freenews.netfront.net";
    host.port = 119;
    host.ssl  = ssl;    

    std::unique_ptr<connection> conn1(new connection("connection1.log", host, commands, responses));
    std::unique_ptr<connection> conn2(new connection("connection2.log", host, commands, responses));
    std::unique_ptr<connection> conn3(new connection("connection3.log", host, commands, responses));

    for (size_t i=0; i<1000; ++i)
    {
        const std::string id = boost::lexical_cast<std::string>(i);
        commands.push_back(new cmd_body(i, i, id, {"mozilla.support.thunderbird"}));
    }

    size_t resp_count = 0;
    while (resp_count < 999)
    {
        auto conn1_status = conn1->wait();
        auto conn2_status = conn2->wait();
        auto conn3_status = conn3->wait();
        auto has_response = responses.wait();

        newsflash::wait_for(conn1_status, conn2_status, conn3_status, has_response);
        if (has_response)
        {
            auto response = responses.get_front();
            auto body = command_cast<cmd_body>(response);
            std::cout << "\nBODY " << body.article() << "\n";
            if (body.status() == cmd_body::cmdstatus::unavailable)
                std::cout << "unavailable\n";
            else if (body.status() == cmd_body::cmdstatus::success)
            {
                //const char* data = (const char*)buffer_data(*body.data);
                std::cout << "success\n";
                //std::cout.write(data + body.data->offset(), body.data->size());
            }
            ++resp_count;
        }
        else if (conn1_status)
        {
            auto status = conn1->get_status();
            if (status.state == connection::state::error)
                conn1.reset(new connection("connection1.log", host, commands, responses));
        }
        else if (conn2_status)
        {
            auto status = conn2->get_status();
            if (status.state == connection::state::error)
                conn2.reset(new connection("connection2.log", host, commands, responses));
        }
        else if (conn3_status)
        {
            auto status = conn3->get_status();
            if (status.state == connection::state::error)
                conn3.reset(new connection("connection3.log", host, commands, responses));
        }
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
    unit_test_multiple_connections(false);
    return 0;
}