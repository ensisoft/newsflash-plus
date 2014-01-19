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
#include "../protocmd.h"

struct string_input {
    const std::string out;
    const std::string in;
    size_t pos;

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

    cmd->cmd_log = cmd_log;
    cmd->cmd_recv = std::bind(&string_input::recv, in, std::placeholders::_1, 
        std::placeholders::_2);
    cmd->cmd_send = std::bind(&string_input::send, in, std::placeholders::_1,
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

    cmd.cmd_log  = cmd_log;
    cmd.cmd_recv = std::bind(&string_input::recv, in, std::placeholders::_1,
        std::placeholders::_2);
    cmd.cmd_send = std::bind(&string_input::send, in, std::placeholders::_1,
        std::placeholders::_2);

    try
    {
        cmd.transact();
        BOOST_REQUIRE(!"exception was expected!");
    }
    catch (const std::exception& e)
    {}

}

void unit_test_cmd_welcome()
{
   test_success(nntp::cmd_welcome{}, "", "200 Welcome posting allowed\r\n", 200);
   test_success(nntp::cmd_welcome{}, "", "201 Welcome - posting prohibited\r\n", 201);
   test_success(nntp::cmd_welcome{}, "", "201 Welcome to Astraweb v. 1.0\r\n", 201);
   test_success(nntp::cmd_welcome{}, "", "200 ok\r\n", 200);

   test_failure(nntp::cmd_welcome{}, "", "440 fooobar\r\n");
   test_failure(nntp::cmd_welcome{}, "", "200 welcome");
}

void unit_test_cmd_auth()
{
   test_success(nntp::cmd_auth_user{"foobar"}, "AUTHINFO USER foobar", "281 authentication accepted\r\n", 281);
   test_success(nntp::cmd_auth_user{"foobar"}, "AUTHINFO USER foobar", "482 authentication accepted\r\n", 482);
   test_failure(nntp::cmd_auth_user{"foobar"}, "AUTHINFO USER foobar", "555, fooobar alalal\r\n");
   test_failure(nntp::cmd_auth_user{"foobar"}, "AUTHINFO USER foobar", "281 authentication accepted");

   test_success(nntp::cmd_auth_pass{"pass"}, "AUTHINFO PASS pass", "281 authentication accepted\r\n", 281);
   test_success(nntp::cmd_auth_pass{"pass"}, "AUTHINFO PASS pass", "481 authentication rejected\r\n", 481);


}

void unit_test_cmd_capabilities()
{
    test_success(nntp::cmd_capabilities{}, "CAPABILITIES", "101 Capability list follows\r\nFOO\r\nBAR\r\n.\r\n", 101);
    test_success(nntp::cmd_capabilities{}, "CAPABILITIES", "101 Capability list follows\r\n.\r\n", 101);
    test_success(nntp::cmd_capabilities{}, "CAPABILITIES", "101 foboaaslas asdglajsd\r\nACTIVE.TIMES.\r\n", 101);
    test_failure(nntp::cmd_capabilities{}, "CAPABILITIES", "200 unxpected response\r\n");
    test_failure(nntp::cmd_capabilities{}, "CAPABILITIES", "101 missing body terminator\r\n");

    nntp::cmd_capabilities cmd;
    test_success(&cmd, "CAPABILITIES", "101 capability list follows\r\nMODE-READER.\r\n", 101);
    BOOST_REQUIRE(cmd.has_mode_reader == true);
    BOOST_REQUIRE(cmd.has_xzver == false);

    cmd.has_mode_reader = false;
    cmd.has_xzver = false;
    test_success(&cmd, "CAPABILITIES", "101 capability list follows\r\nXZVER\r\nMODE-READER\r\n.\r\n", 101);
    BOOST_REQUIRE(cmd.has_mode_reader == true);
    BOOST_REQUIRE(cmd.has_xzver == true);

}

void unit_test_cmd_group()
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
        BOOST_REQUIRE(cmd.article_count == 3);
        BOOST_REQUIRE(cmd.low_water_mark == 100);
        BOOST_REQUIRE(cmd.high_water_mark == 102);
    }
}

void unit_test_scan_response()
{
    {
        int a, b, c;
        BOOST_REQUIRE(nntp::detail::scan_response("123 234254 444", a, b, c));
        BOOST_REQUIRE(a == 123);
        BOOST_REQUIRE(b == 234254);
        BOOST_REQUIRE(c == 444);
    }

    {
        int a; 
        double d;
        BOOST_REQUIRE(nntp::detail::scan_response("200 40.50", a, d));
        BOOST_REQUIRE(a == 200);
        //BOOST_REQUIRE(d == 40.50); // fix
    }

    {
        int a;
        BOOST_REQUIRE(!nntp::detail::scan_response("asdga", a));
    }

    {
        // int a;
        // BOOST_REQUIRE(!nntp::detail::scan_response("233s 23", a));
    }
}

void unit_test_cmd_mode_reader()
{
    test_success(nntp::cmd_mode_reader{}, "MODE READER", "200 posting allowed\r\n", 200);
    test_success(nntp::cmd_mode_reader{}, "MODE READER", "502 BLALBALBH\r\n", 502);
    test_failure(nntp::cmd_mode_reader{}, "MODE READER", "333 foobar\r\n");
    test_failure(nntp::cmd_mode_reader{}, "MODE READER", "asdglsjgs\r\n");
    test_failure(nntp::cmd_mode_reader{}, "MODE READER", "200 blalala");
}

void unit_test_cmd_body()
{
    
}


int test_main(int, char* [])
{
    unit_test_scan_response();

    unit_test_cmd_welcome();
    unit_test_cmd_auth();
    unit_test_cmd_capabilities();
    unit_test_cmd_mode_reader();
    unit_test_cmd_group();

    return 0;
}
