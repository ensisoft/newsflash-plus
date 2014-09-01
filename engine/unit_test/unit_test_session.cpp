// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
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
#include "../session.h"
#include "../buffer.h"

namespace nf = newsflash;

void set(nf::buffer& buff, const char* str)
{
    buff.clear();
    std::strcpy(buff.back(), str);
    buff.append(std::strlen(str));
}

void append(nf::buffer& buff, const char* str)
{
    std::strcpy(buff.back(), str);
    buff.append(std::strlen(str));
}

void unit_test_init_session_success()
{
    nf::session session;
    BOOST_REQUIRE(session.get_error() == nf::session::error::none);
    BOOST_REQUIRE(session.get_state() == nf::session::state::none);

    std::string output;
    session.on_send = [&](const std::string& cmd) {
        output = cmd;
    };
    session.on_auth = [](std::string& user, std::string& pass) {
        user = "foo";
        pass = "bar";
    };

    session.start();

    nf::buffer incoming(1024);
    nf::buffer tmp(1);
    BOOST_REQUIRE(!session.parse_next(incoming, tmp));
    BOOST_REQUIRE(session.get_error() == nf::session::error::none);
    BOOST_REQUIRE(session.get_state() == nf::session::state::init);

    set(incoming, "200 welcome posting allowed\r\n");
    session.parse_next(incoming, tmp);
    BOOST_REQUIRE(output == "CAPABILITIES\r\n");

    set(incoming, "500 what?\r\n");
    session.parse_next(incoming, tmp);
    BOOST_REQUIRE(output == "MODE READER\r\n");

    set(incoming, "480 authentication required\r\n");
    session.parse_next(incoming, tmp);
    BOOST_REQUIRE(output == "AUTHINFO USER foo\r\n");
    BOOST_REQUIRE(session.get_state() == nf::session::state::authenticate);

    set(incoming, "381 password required\r\n");
    session.parse_next(incoming, tmp);
    BOOST_REQUIRE(output == "AUTHINFO PASS bar\r\n");

    set(incoming, "281 authentication accepted\r\n");
    session.parse_next(incoming, tmp);
    BOOST_REQUIRE(output == "MODE READER\r\n");

    set(incoming, "200 posting allowed\r\n");
    session.parse_next(incoming, tmp);
    BOOST_REQUIRE(!session.pending());
    BOOST_REQUIRE(session.get_error() == nf::session::error::none);
    BOOST_REQUIRE(session.get_state() == nf::session::state::ready);

}

void unit_test_init_session_success_caps()
{
    nf::session session;
    BOOST_REQUIRE(session.get_error() == nf::session::error::none);
    BOOST_REQUIRE(session.get_state() == nf::session::state::none);

    std::string output;
    session.on_send = [&](const std::string& cmd) {
        output = cmd;
    };
    session.on_auth = [](std::string& user, std::string& pass) {
        user = "foo";
        pass = "bar";
    };

    session.start();

    nf::buffer incoming(1024);
    nf::buffer tmp(1);

    set(incoming, "200 welcome posting allowed\r\n");
    session.parse_next(incoming, tmp);
    BOOST_REQUIRE(output == "CAPABILITIES\r\n");

    set(incoming, "101 capabilities list follows\r\n"
        "MODE-READER\r\n"
        "XZVER\r\n"
        "IHAVE\r\n"
        "\r\n"
        ".\r\n");
    session.parse_next(incoming, tmp);
    BOOST_REQUIRE(output == "MODE READER\r\n");

    set(incoming, "200 posting allowed\r\n");
    session.parse_next(incoming, tmp);    

    BOOST_REQUIRE(!session.pending()); // done;
    BOOST_REQUIRE(session.has_gzip_compress() == false);
    BOOST_REQUIRE(session.has_xzver() == true);

}

void unit_test_init_session_failure_authenticate()
{
    nf::session session;
    BOOST_REQUIRE(session.get_error() == nf::session::error::none);
    BOOST_REQUIRE(session.get_state() == nf::session::state::none);

    std::string output;
    session.on_send = [&](const std::string& cmd) {
        output = cmd;
    };
    session.on_auth = [](std::string& user, std::string& pass) {
        user = "foo";
        pass = "bar";
    };

    session.start();

    nf::buffer incoming(1024);
    nf::buffer tmp(1);
    set(incoming, "200 welcome posting allowed\r\n");
    session.parse_next(incoming, tmp);
    BOOST_REQUIRE(output == "CAPABILITIES\r\n");

    set(incoming, "500 what?\r\n");
    session.parse_next(incoming, tmp);
    BOOST_REQUIRE(output == "MODE READER\r\n");

    set(incoming, "480 authentication required\r\n");
    session.parse_next(incoming, tmp);
    BOOST_REQUIRE(output == "AUTHINFO USER foo\r\n");
    
    set(incoming, "381 password required\r\n");
    session.parse_next(incoming, tmp);
    BOOST_REQUIRE(output == "AUTHINFO PASS bar\r\n");

    set(incoming, "482 authentication rejected\r\n");
    session.parse_next(incoming, tmp);
    BOOST_REQUIRE(!session.pending());
    BOOST_REQUIRE(session.get_state() == nf::session::state::error);
    BOOST_REQUIRE(session.get_error() == nf::session::error::authentication_rejected);

}

void unit_test_change_group()
{
    nf::session session;

    std::string output;
    session.on_send = [&](const std::string& cmd) {
        output = cmd;
    };

    nf::buffer incoming(1024);
    nf::buffer tmp(1024);

    session.change_group("alt.binaries.foo");

    BOOST_REQUIRE(output == "GROUP alt.binaries.foo\r\n");    

    set(incoming, "211 4 1 4 alt.binaries.foo group succesfully selected\r\n");
    session.parse_next(incoming, tmp);

    BOOST_REQUIRE(!session.pending());
    BOOST_REQUIRE(session.get_state() == nf::session::state::ready);
    BOOST_REQUIRE(session.get_error() == nf::session::error::none);

    BOOST_REQUIRE(tmp.content_type() == nf::buffer::type::groupinfo);
    BOOST_REQUIRE(tmp.content_status() == nf::buffer::status::success);
    BOOST_REQUIRE(tmp.content_length() == std::strlen("211 4 1 4 alt.binaries.foo group succesfully selected\r\n"));
    BOOST_REQUIRE(!std::strncmp(tmp.content(), 
        "211 4 1 4 alt.binaries.foo group succesfully selected\r\n", tmp.content_length()));
}

void unit_test_retrieve_article()
{
    nf::session session;

    std::string output;
    session.on_send = [&](const std::string& cmd) {
        output = cmd;
    };

    {
        nf::buffer incoming(1024);
        nf::buffer content(1024);

        session.retrieve_article("1234");
        BOOST_REQUIRE(output == "BODY 1234\r\n");
        BOOST_REQUIRE(session.get_state() == nf::session::state::transfer);

        set(incoming, "222 body follows\r\n"
            "here's some text data\r\n"
            ".\r\n");
        session.parse_next(incoming, content);
        BOOST_REQUIRE(!session.pending());
        BOOST_REQUIRE(session.get_state() == nf::session::state::ready);
        BOOST_REQUIRE(session.get_error() == nf::session::error::none);

        BOOST_REQUIRE(content.content_type() == nf::buffer::type::article);
        BOOST_REQUIRE(content.content_start() == std::strlen("222 body follows\r\n"));
        BOOST_REQUIRE(content.content_length() == std::strlen("here's some text data\r\n.\r\n"));
        BOOST_REQUIRE(!std::strncmp(content.content(),
            "here's some text data\r\n.\r\n", content.content_length()));
    }

    // multiple enqueued commands
    {
        nf::buffer incoming(1024);
        nf::buffer content(1024);

        session.retrieve_article("1234");
        session.retrieve_article("2345");
        session.retrieve_article("3456");


        set(incoming, "222 body follows\r\nthis is first content\r\n.\r\n");
        BOOST_REQUIRE(output == "BODY 1234\r\n");
        session.parse_next(incoming, content);
        BOOST_REQUIRE(content.content_type() == nf::buffer::type::article);
        BOOST_REQUIRE(content.content_status() == nf::buffer::status::success);
        BOOST_REQUIRE(!std::strncmp(content.content(),
            "this is first content\r\n.\r\n", content.content_length()));

        BOOST_REQUIRE(output == "BODY 2345\r\n");
        set(incoming, "420 no article with that message id dmca takedown\r\n");
        session.parse_next(incoming, content);
        BOOST_REQUIRE(content.content_type() == nf::buffer::type::article);
        BOOST_REQUIRE(content.content_status() == nf::buffer::status::dmca);

        BOOST_REQUIRE(output == "BODY 3456\r\n");
        set(incoming, "423 no article with that number\r\n");
        session.parse_next(incoming, content);
        BOOST_REQUIRE(content.content_type() == nf::buffer::type::article);
        BOOST_REQUIRE(content.content_status() == nf::buffer::status::unavailable);

        BOOST_REQUIRE(!session.pending());
    }

    // pipelining
    { 
        nf::buffer incoming(1024);
        nf::buffer content(1024);

        session.enable_pipelining(true);
        session.on_send = [&](const std::string& cmd) {
            output.append(cmd);
            output.append("/");
        };

        output.clear();

        session.retrieve_article("1234");
        session.retrieve_article("2345");
        session.retrieve_article("3456");
        BOOST_REQUIRE(output == "BODY 1234\r\n/BODY 2345\r\n/BODY 3456\r\n/");

        set(incoming, "222 body follows\r\nthis is first content\r\n.\r\n"
            "222 body fol");

        BOOST_REQUIRE(session.parse_next(incoming, content));
        BOOST_REQUIRE(content.content_type() == nf::buffer::type::article);
        BOOST_REQUIRE(!std::strncmp(content.content(),
            "this is first content\r\n.\r\n", content.content_length()));

        append(incoming, "lows\r\nsecond content\r\n.\r\n"
            "423 no such article with that number\r\n");

        BOOST_REQUIRE(session.parse_next(incoming, content));
        BOOST_REQUIRE(!std::strncmp(content.content(),
            "second content\r\n.\r\n", content.content_length()));

        BOOST_REQUIRE(session.parse_next(incoming, content));
        BOOST_REQUIRE(content.content_status() == nf::buffer::status::unavailable);
    }

}

int test_main(int, char*[])
{
    unit_test_init_session_success();
    unit_test_init_session_success_caps();
    unit_test_init_session_failure_authenticate();
    unit_test_change_group();
    unit_test_retrieve_article();
    return 0;
}