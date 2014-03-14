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
#include "../cmd_welcome.h"
#include "../cmd_auth_user.h"
#include "../cmd_auth_pass.h"
#include "../cmd_capabilities.h"
#include "../cmd_group.h"
#include "../cmd_mode_reader.h"
#include "../cmd_xover.h"
#include "../cmd_xzver.h"
#include "../cmd_list.h"
#include "../cmd_body.h"

struct string_input {
    const std::string out;
    const std::string in;
    std::size_t pos;

    string_input(const std::string& output, const std::string& input) 
        : out(output), in(input), pos(0)
    {}

    size_t recv(void* buff, size_t buffsize)
    {
        const auto m = std::min(buffsize, in.size() - pos);
        std::memcpy(buff, &in[pos], m);
        pos += m;
        return m;
    }
    void send(const void* cmd, size_t cmdsize) 
    {
        std::string str((char*)cmd, cmdsize);
        BOOST_REQUIRE(str == out + "\r\n");
    }
};

void cmd_log(const std::string& str)
{
    std::cout << str << std::endl;
}

template<typename Cmd>
void test_success(Cmd* cmd, const char* request, const char* response, int code)
{
    string_input in {request, response};

    cmd->log  = cmd_log;
    cmd->recv = std::bind(&string_input::recv, in, std::placeholders::_1, 
        std::placeholders::_2);
    cmd->send = std::bind(&string_input::send, in, std::placeholders::_1,
        std::placeholders::_2);

    int ret = cmd->transact();
    BOOST_REQUIRE(ret == code);
}

template<typename Cmd>
void test_success(Cmd cmd, const char* request, const char* response, int code)
{
    test_success(&cmd, request, response, code);
}

template<typename Cmd>
void test_failure(Cmd cmd, const char* request, const char* response)
{
    string_input in {request, response};

    cmd.log  = cmd_log;
    cmd.recv = std::bind(&string_input::recv, in, std::placeholders::_1,
        std::placeholders::_2);
    cmd.send = std::bind(&string_input::send, in, std::placeholders::_1,
        std::placeholders::_2);

    try
    {
        cmd.transact();
        BOOST_REQUIRE(!"exception was expected!");
    }
    catch (const std::exception& e)
    {}
}


void test_cmd_welcome()
{
   test_success(nntp::cmd_welcome{}, "", "200 Welcome posting allowed\r\n", 200);
   test_success(nntp::cmd_welcome{}, "", "201 Welcome - posting prohibited\r\n", 201);
   test_success(nntp::cmd_welcome{}, "", "201 Welcome to Astraweb v. 1.0\r\n", 201);
   test_success(nntp::cmd_welcome{}, "", "200 ok\r\n", 200);

   test_failure(nntp::cmd_welcome{}, "", "440 fooobar\r\n");
   test_failure(nntp::cmd_welcome{}, "", "200 welcome");
}

void test_cmd_auth()
{
   test_success(nntp::cmd_auth_user{"foobar"}, "AUTHINFO USER foobar", "281 authentication accepted\r\n", 281);
   test_success(nntp::cmd_auth_user{"foobar"}, "AUTHINFO USER foobar", "482 authentication accepted\r\n", 482);
   test_failure(nntp::cmd_auth_user{"foobar"}, "AUTHINFO USER foobar", "555, fooobar alalal\r\n");
   test_failure(nntp::cmd_auth_user{"foobar"}, "AUTHINFO USER foobar", "281 authentication accepted");

   test_success(nntp::cmd_auth_pass{"pass"}, "AUTHINFO PASS pass", "281 authentication accepted\r\n", 281);
   test_success(nntp::cmd_auth_pass{"pass"}, "AUTHINFO PASS pass", "481 authentication rejected\r\n", 481);
}

void test_cmd_pass()
{
    // todo:
}

void test_cmd_caps()
{
    test_success(nntp::cmd_capabilities{}, "CAPABILITIES", "101 Capability list follows\r\nFOO\r\nBAR\r\n.\r\n", 101);
    test_success(nntp::cmd_capabilities{}, "CAPABILITIES", "101 Capability list follows\r\n\r\n.\r\n", 101);
    test_success(nntp::cmd_capabilities{}, "CAPABILITIES", "101 foboaaslas asdglajsd\r\nACTIVE.TIMES\r\n.\r\n", 101);
    test_failure(nntp::cmd_capabilities{}, "CAPABILITIES", "200 unxpected response\r\n");
    test_failure(nntp::cmd_capabilities{}, "CAPABILITIES", "101 missing body terminator\r\n");

    nntp::cmd_capabilities cmd;
    test_success(&cmd, "CAPABILITIES", "101 capability list follows\r\nMODE-READER\r\n.\r\n", 101);
    BOOST_REQUIRE(cmd.has_mode_reader == true);
    BOOST_REQUIRE(cmd.has_xzver == false);

    cmd.has_mode_reader = false;
    cmd.has_xzver = false;
    test_success(&cmd, "CAPABILITIES", "101 capability list follows\r\nXZVER\r\nMODE-READER\r\n.\r\n", 101);
    BOOST_REQUIRE(cmd.has_mode_reader == true);
    BOOST_REQUIRE(cmd.has_xzver == true);    
}

void test_cmd_group()
{
    {
        test_success(nntp::cmd_group{"alt.binaries.foo"}, "GROUP alt.binaries.foo", "411 no such group\r\n", 411);

        test_failure(nntp::cmd_group{"alt.binaries.foo"}, "GROUP alt.binaries.foo", "444 foobar\r\n");
        test_failure(nntp::cmd_group{"alt.binaries.foo"}, "GROUP alt.binaries.foo", "alsjglas");
        test_failure(nntp::cmd_group{"alt.binaries.foo"}, "GROUP alt.binaries.foo", "1234");
        test_failure(nntp::cmd_group{"alt.binaries.foo"}, "GROUP alt.binaries.foo", "211 323542\r\n");
        test_failure(nntp::cmd_group{"alt.binaries.foo"}, "GROUP alt.binaries.foo", "211 0 1 ssagas\r\n");

    }

    {
        nntp::cmd_group cmd {"alt.binaries.bar"};
        test_success(&cmd, "GROUP alt.binaries.bar", "211 3 100 102 alt.binaries.bar Group selected\r\n", 211);
        BOOST_REQUIRE(cmd.count == 3);
        BOOST_REQUIRE(cmd.low == 100);
        BOOST_REQUIRE(cmd.high == 102);
    }
}

void test_cmd_mode_reader()
{
    test_success(nntp::cmd_mode_reader{}, "MODE READER", "200 posting allowed\r\n", 200);
    test_success(nntp::cmd_mode_reader{}, "MODE READER", "502 BLALBALBH\r\n", 502);
    test_failure(nntp::cmd_mode_reader{}, "MODE READER", "333 foobar\r\n");
    test_failure(nntp::cmd_mode_reader{}, "MODE READER", "asdglsjgs\r\n");
    test_failure(nntp::cmd_mode_reader{}, "MODE READER", "200 blalala");
}

void test_cmd_xover()
{
    // todo:
}

void test_cmd_xzver()
{
    // todo:
}

void test_cmd_body()
{
    typedef std::vector<char> buffer_t;
    typedef nntp::cmd_body<buffer_t> command_t;

    {
        buffer_t buff(1024);

        command_t cmd {buff, "1234"};

        test_failure(cmd, "BODY 1234", "333 foobar\r\n");
        test_failure(cmd, "BODY 1234", "223 FOOBAR\r\n");
        test_failure(cmd, "BODY 1234", "asgasgas\r\n");
    }

    {
        buffer_t buff(1024);

        command_t cmd {buff, "1234"};

        test_success(&cmd, "BODY 1234", "420 no article with that message id\r\n", 420);
        test_success(&cmd, "BODY 1234", "222 body follows\r\n\r\n.\r\n", 222);
        BOOST_REQUIRE(cmd.size == 0);

        test_success(&cmd, "BODY 1234", "222 body follows\r\nfoobar\r\n.\r\n", 222);
        BOOST_REQUIRE(cmd.size == strlen("foobar"));
        BOOST_REQUIRE(cmd.offset == 18);
        BOOST_REQUIRE(!std::strncmp(&buff[cmd.offset], "foobar", cmd.size));
    }

    {
        buffer_t buff(20);

        command_t cmd {buff, "1234"};

        const char* body = 
           "assa sassa mandelmassa\r\n"
           "second line of data\r\n"
           "foobar"
           "\r\n"
           "."
           "\r\n";

        std::string str("222 body follows\r\n");
        str.append(body);
        test_success(&cmd, "BODY 1234", str.c_str(), 222);

        BOOST_REQUIRE(cmd.offset == std::strlen("222 body follows\r\n"));
        BOOST_REQUIRE(cmd.size   == std::strlen(body) - 5);
        BOOST_REQUIRE(buff.size() >= cmd.size);
    }    
}

void test_cmd_list()
{
    typedef std::vector<char> buffer_t;
    typedef nntp::cmd_list<buffer_t> command_t;

    // test success
    {
        buffer_t buff(1024);
        command_t cmd {buff};

        test_success(&cmd, "LIST", "215 list of newsgroups follows\r\n\r\n.\r\n", 215);
        BOOST_REQUIRE(cmd.size == 0);

        test_success(&cmd, "LIST", 
            "215 list of newsgroups follows\r\n"
            "misc.test 3002322 3000234 y\r\n"            
            "comp.risks 442001 441099 m\r\n"
            "\r\n"
            "."
            "\r\n", 
            215);

        BOOST_REQUIRE(cmd.size == 
            std::strlen("misc.test 3002322 3000234 y\r\n"
                        "comp.risks 442001 441099 m\r\n"));
    }    
    
}

int test_main(int, char*[])
{
    test_cmd_welcome();
    test_cmd_auth();
    test_cmd_pass();
    test_cmd_caps();
    test_cmd_group();
    test_cmd_mode_reader();
    test_cmd_xover();
    test_cmd_xzver();
    test_cmd_body();
    test_cmd_list();
    return 0;
}