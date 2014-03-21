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
#include "../protocol.h"
#include "../buffer.h"

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

void cmd_log(const std::string& str)
{
    std::cout << str << std::endl;
}

void test_handshake()
{
    // test succesful handshakes
    {
        test_sequence test;
        test.responses = 
        {
            "200 service available, posting allowed",
            "500 what?", 
            "200 OK",

            "201 service available posting prohibited",
            "500 WHAT?",
            "200 OK"
        };

        test.commands = 
        {
            "CAPABILITIES",
            "MODE READER",

            "CAPABILITIES",
            "MODE READER"
        };

        engine::protocol proto;
        proto.on_recv = std::bind(&test_sequence::recv, &test, std::placeholders::_1, std::placeholders::_2);
        proto.on_send = std::bind(&test_sequence::send, &test,std::placeholders::_1, std::placeholders::_2);
        proto.on_log  = cmd_log;

        proto.connect();
        proto.connect();
    }

    // test failing handshakes
    {
        test_sequence test;
        test.responses = 
        {
            "400 service temporarily unavailable",
            "502 service permanently unavailable",
            "5555 foobar"
        };

        engine::protocol proto;
        proto.on_recv = std::bind(&test_sequence::recv, &test, std::placeholders::_1, std::placeholders::_2);
        proto.on_send = std::bind(&test_sequence::send, &test,std::placeholders::_1, std::placeholders::_2);
        proto.on_log  = cmd_log;


        REQUIRE_EXCEPTION(proto.connect());
        REQUIRE_EXCEPTION(proto.connect());
        REQUIRE_EXCEPTION(proto.connect());
    }
}

void test_authentication()
{
    test_sequence test;
    test.password = "pass123";
    test.username = "user123";

    engine::protocol proto;
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
                "",
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
        engine::buffer buff;
        buff.reserve(1024);

        proto.connect();
        REQUIRE_EXCEPTION(proto.body("<1>", buff));
    }

    // accepted authentication
    {
        test.responses =
        {
            "200 welcome posting allowed",
            "480 authentication required",
            "281 authentication accepted",                
            "101 capabilities list follows",
                "",
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
        engine::buffer buff;
        buff.reserve(100);

        proto.connect();
        BOOST_REQUIRE(!proto.group("alt.foo.bar"));
        proto.quit();
    }

    // accepted authentication
    {
        test.responses = 
        {
            "200 welcome posting allowed",
            "480 authentication required",
            "381 password required",
            "281 authentication accepted",
            "101 capabilities list follows",
                "",
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
        engine::buffer buff;
        buff.reserve(100);

        proto.connect();
        BOOST_REQUIRE(!proto.group("alt.foo.bar"));
        proto.quit();
    }
}

template<typename T>
std::string to_string(const T& t)
{
    return std::string(t.data(), t.size());
}

void test_listing()
{
    // LIST command has not been specified to fail

    // empty list
    {
        test_sequence test;
        test.responses = 
        {
            "215 list of newsgroups follows",
                "."
        };
        test.commands = 
        {
            "LIST"
        };

        engine::protocol proto;
        proto.on_recv = std::bind(&test_sequence::recv, &test, std::placeholders::_1, std::placeholders::_2);
        proto.on_send = std::bind(&test_sequence::send, &test, std::placeholders::_1, std::placeholders::_2);

        engine::buffer buff;

        BOOST_REQUIRE(proto.list(buff));

        const engine::buffer::header header(buff);
        const engine::buffer::payload body(buff);

        BOOST_REQUIRE(to_string(header) == "215 list of newsgroups follows\r\n");
        BOOST_REQUIRE(body.empty());
    }

    // non-empty list
    {
        test_sequence test;
        test.responses = 
        {
            "215 list of newsgroups follows",
                "alt.binaries.foo 1 2 y",
                "alt.binaries.bar 3 4 y",
                "."
        };
        test.commands = 
        {
            "LIST"
        };

        engine::protocol proto;
        proto.on_recv = std::bind(&test_sequence::recv, &test, std::placeholders::_1, std::placeholders::_2);
        proto.on_send = std::bind(&test_sequence::send, &test, std::placeholders::_1, std::placeholders::_2);
        
        engine::buffer buff;

        BOOST_REQUIRE(proto.list(buff));

        const engine::buffer::header header(buff);
        const engine::buffer::payload body(buff);

        BOOST_REQUIRE(to_string(header) == "215 list of newsgroups follows\r\n");
        BOOST_REQUIRE(to_string(body) == "alt.binaries.foo 1 2 y\r\n"
                                         "alt.binaries.bar 3 4 y\r\n");
    }
}

void test_body()
{
    // unavailable
    {
        test_sequence test;
        test.responses = 
        {
            "420 no article with that message-id"
        };
        test.commands =
        {
            "BODY 1234"
        };


        engine::protocol proto;
        proto.on_recv = std::bind(&test_sequence::recv, &test, std::placeholders::_1, std::placeholders::_2);
        proto.on_send = std::bind(&test_sequence::send, &test, std::placeholders::_1, std::placeholders::_2);        

        engine::buffer buff;

        BOOST_REQUIRE(proto.body("1234", buff) == engine::protocol::status::unavailable);
    }

    // available, non empty
    {
        test_sequence test;
        test.responses = 
        {
            "222 body follows",
                "foobar",
                "assa sassa mandelmassa",
                "."
        };

        test.commands = 
        {
            "BODY 1234"
        };

        engine::protocol proto;
        proto.on_recv = std::bind(&test_sequence::recv, &test, std::placeholders::_1, std::placeholders::_2);
        proto.on_send = std::bind(&test_sequence::send, &test, std::placeholders::_1, std::placeholders::_2);                

        engine::buffer buff;
        BOOST_REQUIRE(proto.body("1234", buff) == engine::protocol::status::success);

        const engine::buffer::header header(buff);
        const engine::buffer::payload body(buff);

        BOOST_REQUIRE(to_string(header) == "222 body follows\r\n");
        BOOST_REQUIRE(to_string(body) == "foobar\r\nassa sassa mandelmassa\r\n");
    }

    // available, empty
    {
        test_sequence test;
        test.responses = 
        {
            "222 body follows",
               "."
        };

        test.commands = 
        {
            "BODY 1234"
        };       

        engine::protocol proto;
        proto.on_recv = std::bind(&test_sequence::recv, &test, std::placeholders::_1, std::placeholders::_2);
        proto.on_send = std::bind(&test_sequence::send, &test, std::placeholders::_1, std::placeholders::_2);                

        engine::buffer buff;
        BOOST_REQUIRE(proto.body("1234", buff) == engine::protocol::status::success);

        const engine::buffer::header header(buff);
        const engine::buffer::payload body(buff);

        BOOST_REQUIRE(to_string(header) == "222 body follows\r\n");
        BOOST_REQUIRE(body.empty());
    }

}

void test_xover()
{
    // todo
}

void test_api_sequence()
{
    // test api functions to request data from the server
    // like in a real system run setting.

    test_sequence test;
    test.responses = 
    {
        "200 welcome posting allowed",
        "101 capabilities list follows",
        "MODE-READER",
        "XZVER",
        "IHAVE",
        "",
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
        "",
        ".",
        "215 list of newsgroups follows",
        "misc.test 3 4 y",
        "comp.risks 4 5 m",
        "",
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

    engine::protocol proto;
    proto.on_recv = std::bind(&test_sequence::recv, &test, std::placeholders::_1, std::placeholders::_2);
    proto.on_send = std::bind(&test_sequence::send, &test, std::placeholders::_1, std::placeholders::_2);
    proto.on_log  = cmd_log;

    proto.connect();
    BOOST_REQUIRE(!proto.group("foo.bar.baz"));
    BOOST_REQUIRE(proto.group("blah.bluh"));

    engine::protocol::groupinfo info {0};
    BOOST_REQUIRE(proto.group("test.test", info));
    BOOST_REQUIRE(info.high_water_mark == 4);
    BOOST_REQUIRE(info.low_water_mark == 1);
    BOOST_REQUIRE(info.article_count == 3);

    engine::buffer buff;
    buff.reserve(100);

    BOOST_REQUIRE(proto.body("<1>", buff) == engine::protocol::status::dmca);
    BOOST_REQUIRE(proto.body("<2>", buff) == engine::protocol::status::unavailable);
    BOOST_REQUIRE(proto.body("<3>", buff) == engine::protocol::status::success);

    BOOST_REQUIRE(proto.list(buff));

    proto.quit();
}

int test_main(int, char* [])
{
    test_handshake();
    test_authentication();
    test_listing();
    test_body();
    test_xover();
    test_api_sequence();

    return 0;
}
