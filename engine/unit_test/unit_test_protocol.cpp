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
#include "unit_test_common.h"
#include "../protocmd.h"
#include "../protocol.h"
#include "../buffer.h"

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

void unit_test_find_response()
{
    {
        BOOST_REQUIRE(nntp::detail::find_response("", 0) == 0);
        BOOST_REQUIRE(nntp::detail::find_response("adsgas", 6) == 0);
        BOOST_REQUIRE(nntp::detail::find_response("\r\n", 2) == 2);
        BOOST_REQUIRE(nntp::detail::find_response("foo\r\n", 5) == 5);
        BOOST_REQUIRE(nntp::detail::find_response("foo\r\nfoo", 5) == 5);
    }

    {
        BOOST_REQUIRE(nntp::detail::find_body("", 0) == 0);
        BOOST_REQUIRE(nntp::detail::find_body("asgas", 5) == 0);
        BOOST_REQUIRE(nntp::detail::find_body(".\r\n", 3) == 3);
        BOOST_REQUIRE(nntp::detail::find_body("foobar.\r\n", 9) == 9);
        BOOST_REQUIRE(nntp::detail::find_body("foobar.\r\nkeke", 13) == 9);
        BOOST_REQUIRE(nntp::detail::find_body("foo\r\nbar.\r\n", 11) == 11);
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
        std::string s;
        BOOST_REQUIRE(nntp::detail::scan_response("233 welcome posting allowed", a, s));
        BOOST_REQUIRE(a == 233);
        BOOST_REQUIRE(s == "welcome");
    }

    {
        int a;
        nntp::detail::trailing_comment c;
        BOOST_REQUIRE(nntp::detail::scan_response("233 welcome posting allowed", a, c));
        BOOST_REQUIRE(a == 233);
        BOOST_REQUIRE(c.str == "welcome posting allowed");
    }

    {
        int a;
        BOOST_REQUIRE(!nntp::detail::scan_response("asdga", a));
    }


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
        BOOST_REQUIRE(cmd.count == 3);
        BOOST_REQUIRE(cmd.low == 100);
        BOOST_REQUIRE(cmd.high == 102);
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
        test_success(&cmd, "BODY 1234", "222 body follows\r\n.\r\n", 222);
        BOOST_REQUIRE(cmd.size == 0);

        test_success(&cmd, "BODY 1234", "222 body follows\r\nfoobar.\r\n", 222);
        BOOST_REQUIRE(cmd.size == 6);
        BOOST_REQUIRE(cmd.offset == 18);
        BOOST_REQUIRE(!std::strncmp(&buff[cmd.offset], "foobar", cmd.size));
    }

    {
        buffer_t buff(20);

        command_t cmd {buff, "1234"};

        const char* body = 
           "assa sassa mandelmassa\r\n"
           "second line of data\r\n"
           "foobar\r\n"
           ".\r\n";

        std::string str("222 body follows\r\n");
        str.append(body);
        test_success(&cmd, "BODY 1234", str.c_str(), 222);

        BOOST_REQUIRE(cmd.offset == std::strlen("222 body follows\r\n"));
        BOOST_REQUIRE(cmd.size   == std::strlen(body) - 3);
        BOOST_REQUIRE(buff.size() >= cmd.size);
    }
}

void unit_test_cmd_xover()
{

}

void unit_test_cmd_xzver()
{

}

void unit_test_cmd_list()
{
    typedef std::vector<char> buffer_t;
    typedef nntp::cmd_list<buffer_t> command_t;

    // test success
    {
        buffer_t buff(1024);
        command_t cmd {buff};

        test_success(&cmd, "LIST", "215 list of newsgroups follows\r\n.\r\n", 215);
        BOOST_REQUIRE(cmd.size == 0);

        test_success(&cmd, "LIST", 
            "215 list of newsgroups follows\r\n"
            "misc.test 3002322 3000234 y\r\n"            
            "comp.risks 442001 441099 m\r\n"
            ".\r\n", 
            215);

        BOOST_REQUIRE(cmd.size == 
            std::strlen("misc.test 3002322 3000234 y\r\n"
                        "comp.risks 442001 441099 m\r\n"));
    }    
}

struct test_sequence {
    std::deque<std::string> commands;
    std::deque<std::string> responses;
    std::string username;
    std::string password;

    size_t recv(void* buff, size_t capacity)
    {
        BOOST_REQUIRE(!responses.empty());
        auto next = responses[0];
        next.append("\r\n");
        BOOST_REQUIRE(capacity >= next.size());
        std::memcpy(buff, &next[0], next.size());
        responses.pop_front();

        return next.size();
    }

    void send(const void* data, size_t len)
    {
        BOOST_REQUIRE(!commands.empty());
        auto next = commands[0];
        next.append("\r\n");
        BOOST_REQUIRE(next.size() == len);
        BOOST_REQUIRE(!std::memcmp(&next[0], data, len));
        commands.pop_front();
    }

    void authenticate(std::string& user, std::string& pass)
    {
        user = username;
        pass = password;
    }

};

void unit_test_protocol()
{
    // test failing handshakes
    {
        test_sequence test;
        test.responses = 
        {
            "400 service temporarily unavailable",
            "502 service permanently unavailable",
            "5555 foobar"
        };

        newsflash::protocol proto;
        proto.on_recv = std::bind(&test_sequence::recv, &test, std::placeholders::_1, std::placeholders::_2);
        proto.on_send = std::bind(&test_sequence::send, &test,std::placeholders::_1, std::placeholders::_2);
        proto.on_log  = cmd_log;


        REQUIRE_EXCEPTION(proto.connect());
        REQUIRE_EXCEPTION(proto.connect());
        REQUIRE_EXCEPTION(proto.connect());
    }

    // test authencation
    {
        test_sequence test;
        test.password = "pass123";
        test.username = "user123";

        newsflash::protocol proto;
        proto.on_recv = std::bind(&test_sequence::recv, &test, std::placeholders::_1, std::placeholders::_2);
        proto.on_send = std::bind(&test_sequence::send, &test, std::placeholders::_1, std::placeholders::_2);
        proto.on_auth = std::bind(&test_sequence::authenticate, &test, std::placeholders::_1, std::placeholders::_2);
        proto.on_log = cmd_log;

        // rejected authentication
        {
            test.responses = 
            {
                "200 welcome posting allowed",
                "480 authentication required",
                "481 authentication rejected"
            };

            test.commands = 
            {
                "CAPABILITIES",
                "AUTHINFO USER user123"
            };
            REQUIRE_EXCEPTION(proto.connect());
        }

        // rejected authentication
        {

            test.responses = 
            {
                "200 welcome posting allowed",
                "480 authenticdation required",
                "381 password required",
                "481 authentication rejected"
            };
            test.commands =
            {
                "CAPABILITIES",
                "AUTHINFO USER user123",
                "AUTHINFO PASS pass123",
            };
            REQUIRE_EXCEPTION(proto.connect());
        }
        
        // rejected authentication
        {
            test.responses = 
            {
                "200 welcome posting allowed",
                "101 capabilities list follows",
                    "MODE-READER",
                    ".",
                "200 posting allowed",
                "480 authentication required",
                "481 authentication rejected"
            };
            test.commands = 
            {
                "CAPABILITIES",
                "MODE READER",
                "BODY <1>",
                "AUTHINFO USER user123"
            };
            newsflash::buffer buff;
            buff.allocate(1024);

            proto.connect();
            REQUIRE_EXCEPTION(proto.download_article("<1>", buff));
        }

        // accepted authentication
        {
            test.responses =
            {
                "200 welcome posting allowed",
                "480 authentication required",
                "281 authentication accepted",                
                "101 capabilities list follows",
                    ".",
                "201 posting prohibited",
                "411 no such newsgroup",
                "205 goodbye"
            };
            test.commands =
            {
                "CAPABILITIES",
                "AUTHINFO USER user123",                
                "CAPABILITIES",
                "MODE READER",
                "GROUP alt.foo.bar",
                "QUIT"
            };
            newsflash::buffer buff;
            buff.allocate(100);

            proto.connect();
            BOOST_REQUIRE(!proto.change_group("alt.foo.bar"));
            proto.quit();
        }

        {
            test.responses = 
            {
                "200 welcome posting allowed",
                "480 authentication required",
                "381 password required",
                "281 authentication accepted",
                "101 capabilities list follows",
                    ".",
                "200 ok posting allowed",
                "411 no such newsgroup",
                "205 goodbye"
            };
            test.commands = 
            {
                "CAPABILITIES",
                "AUTHINFO USER user123",
                "AUTHINFO PASS pass123",
                "CAPABILITIES",
                "MODE READER",
                "GROUP alt.foo.bar",
                "QUIT"
            };
            newsflash::buffer buff;
            buff.allocate(100);

            proto.connect();
            BOOST_REQUIRE(!proto.change_group("alt.foo.bar"));
            proto.quit();
        }

    }


    // test api functions to request data from the server
    {
        test_sequence test;
        test.responses = 
        {
            "200 welcome posting allowed",
            "101 capabilities list follows",
                "MODE-READER",
                "XZVER",
                "IHAVE",
                ".",
            "200 posting allowed",
            "411 no such newsgroup",
            "211 3 1 4 blah.bluh",
            "211 3 1 4 test.test",
            "420 dmca takedown",
            "420 no such article",
            "222 body follows",
               "foobarlasg",
               "kekekeke",
               ".",
            "215 list of newsgroups follows",
               "misc.test 3 4 y",
               "comp.risks 4 5 m",
               ".",
           "205 goodbye"
        };
        test.commands = 
        {
            "CAPABILITIES",
            "MODE READER",
            "GROUP foo.bar.baz",
            "GROUP blah.bluh",
            "GROUP test.test",
            "BODY <1>",
            "BODY <2>",
            "BODY <3>",
            "LIST",
            "QUIT"
        };

        newsflash::protocol proto;
        proto.on_recv = std::bind(&test_sequence::recv, &test, std::placeholders::_1, std::placeholders::_2);
        proto.on_send = std::bind(&test_sequence::send, &test, std::placeholders::_1, std::placeholders::_2);
        proto.on_log  = cmd_log;

        proto.connect();
        BOOST_REQUIRE(!proto.change_group("foo.bar.baz"));
        BOOST_REQUIRE(proto.change_group("blah.bluh"));

        newsflash::protocol::groupinfo info {0};
        BOOST_REQUIRE(proto.query_group("test.test", info));
        BOOST_REQUIRE(info.high_water_mark == 4);
        BOOST_REQUIRE(info.low_water_mark == 1);
        BOOST_REQUIRE(info.article_count == 3);

        newsflash::buffer buff;
        buff.allocate(100);

        BOOST_REQUIRE(proto.download_article("<1>", buff) == newsflash::protocol::status::dmca);
        BOOST_REQUIRE(proto.download_article("<2>", buff) == newsflash::protocol::status::unavailable);
        BOOST_REQUIRE(proto.download_article("<3>", buff) == newsflash::protocol::status::success);

        BOOST_REQUIRE(proto.download_list(buff));

        proto.quit();
    }


}

int test_main(int, char* [])
{
    unit_test_scan_response();
    unit_test_find_response();
    unit_test_cmd_welcome();
    unit_test_cmd_auth();
    unit_test_cmd_capabilities();
    unit_test_cmd_mode_reader();
    unit_test_cmd_group();
    unit_test_cmd_body();
    unit_test_cmd_xover();
    unit_test_cmd_xzver();
    unit_test_cmd_list();
    unit_test_protocol();

    return 0;
}
