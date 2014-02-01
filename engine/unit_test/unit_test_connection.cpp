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
#include "../connection.h"
#include "../tcpsocket.h"
#include "../sslsocket.h"
#include "../command.h"
#include "../waithandle.h"
#include "../buffer.h"

using namespace newsflash;

void unit_test_connection_resolve_error(bool ssl)
{
    cmdqueue cmds;
    resqueue resp;

    connection::server host;
    host.addr = "asdgljasljgas";
    host.port = 8888;
    host.ssl  = ssl;

    connection conn("clog", host, cmds, resp);
    wait(conn);

    auto status = conn.get_status();
    BOOST_REQUIRE(status.state == connection::state::error);
    BOOST_REQUIRE(status.error == connection::error::resolve);
}

void unit_test_connection_refused(bool ssl)
{
    cmdqueue cmds;
    resqueue resp;

    connection::server host;
    host.addr = "localhost";
    host.port = 2119;
    host.ssl  = ssl;

    connection conn("clog", host, cmds, resp);
    wait(conn);

    auto status = conn.get_status();
    BOOST_REQUIRE(status.state == connection::state::error);
    BOOST_REQUIRE(status.error == connection::error::refused);
}

void unit_test_connection_interrupted(bool ssl)
{
    cmdqueue cmds;
    resqueue resp;

    connection::server host;
    host.addr = "news.budgetnews.net";
    host.port = 199;
    host.ssl  = false;

    connection* conn = new connection("clog", host, cmds, resp);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    delete conn;
}

void unit_test_connection_forbidden(bool ssl)
{
    cmdqueue cmds;
    resqueue resp;

    connection::server host;
    host.addr = "news.budgetnews.net";
    host.port = 119;
    host.ssl  = false;
    host.pass = "foobar";
    host.user = "foobar";

    connection conn("clog", host, cmds, resp);
    wait(conn);

    auto status = conn.get_status();
    BOOST_REQUIRE(status.state == connection::state::error);
    BOOST_REQUIRE(status.error == connection::error::forbidden);
}

void unit_test_connection_timeout_error(bool ssl)
{
    cmdqueue cmds;
    resqueue resp;

    connection::server host;
    host.addr = "www.google.com";
    host.port = 80;
    host.ssl  = false;

    connection conn("clog", host, cmds, resp);
    wait(conn);

    auto status = conn.get_status();
    BOOST_REQUIRE(status.state == connection::state::error);
    BOOST_REQUIRE(status.error == connection::error::timeout);
}

void unit_test_connection_protocol_error(bool ssl)
{
    cmdqueue cmds;
    resqueue resp;

    connection::server host;
    // todo:
}

void unit_test_connection_success(bool ssl)
{
    cmdqueue commands;
    resqueue responses;

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
        cmd_list cmd;
        cmd.id     = 123;
        cmd.taskid = 444;
        cmd.size   = 1024 * 5;
        commands.post_back(cmd_list::ID, cmd);

        wait(responses);
        auto resp = responses.get_front();

        cmd_list& ret = resp->as<cmd_list>();
        BOOST_REQUIRE(ret.id == cmd.id);
        BOOST_REQUIRE(ret.taskid == cmd.taskid);
    }

    // no such group
    {
        cmd_group cmd;
        cmd.name   = "mozilla.support.thunderbird-blalala";
        cmd.id     = 1;
        cmd.taskid = 2;
        cmd.success = true;
        cmd.article_count = cmd.high_water_mark = cmd.low_water_mark = 0;
        commands.post_back(cmd_group::ID, cmd);        

        wait(responses);
        auto resp = responses.get_front();

        cmd_group& ret = resp->as<cmd_group>();

        BOOST_REQUIRE(ret.id == cmd.id);
        BOOST_REQUIRE(ret.taskid == cmd.taskid);
        BOOST_REQUIRE(ret.success == false);
        BOOST_REQUIRE(ret.article_count == 0);
        BOOST_REQUIRE(ret.low_water_mark == 0);
        BOOST_REQUIRE(ret.high_water_mark == 0);
    }

    // a valid group
    {
        cmd_group cmd;
        cmd.name   = "mozilla.support.thunderbird";
        cmd.id     = 1;
        cmd.taskid = 2;
        commands.post_back(cmd_group::ID, cmd);

        wait(responses);
        auto resp = responses.get_front();

        cmd_group& ret = resp->as<cmd_group>();

        BOOST_REQUIRE(ret.success);
        BOOST_REQUIRE(ret.article_count);
        BOOST_REQUIRE(ret.high_water_mark);
        BOOST_REQUIRE(ret.low_water_mark);
    }

    // no such group for xover
    {
        cmd_xover cmd;
        cmd.size = 1024;
        cmd.success = true;
        cmd.group = "mozilla.support.thunderbird.blala";
        commands.post_back(cmd_xover::ID, cmd);

        wait(responses);
        auto resp = responses.get_front();

        cmd_xover& ret = resp->as<cmd_xover>();
        BOOST_REQUIRE(ret.success == false);
    }

    // succesful xover
    {
        cmd_xover cmd;
        cmd.size = 1024;
        cmd.success = false;
        cmd.group = "mozilla.support.thunderbird";
        cmd.start = 1;
        cmd.end   = 100000;
        commands.post_back(cmd_xover::ID, cmd);

        wait(responses);
        auto resp = responses.get_front();

        cmd_xover& ret = resp->as<cmd_xover>();

        BOOST_REQUIRE(ret.success);
        BOOST_REQUIRE(ret.data->size());
    }

    // todo: body
}

int test_main(int argc, char* argv[])
{
    unit_test_connection_resolve_error(false);
    unit_test_connection_refused(false);
    unit_test_connection_interrupted(false);
    unit_test_connection_forbidden(false);
    unit_test_connection_timeout_error(false);

    unit_test_connection_success(false);

    return 0;
}