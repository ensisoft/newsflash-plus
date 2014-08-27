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

    nf::buffer incoming;
    nf::buffer tmp;
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

    nf::buffer incoming;
    nf::buffer tmp;

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

    nf::buffer incoming;
    nf::buffer tmp;
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
    BOOST_REQUIRE(session.pending());
    BOOST_REQUIRE(session.get_state() == nf::session::state::error);
    BOOST_REQUIRE(session.get_error() == nf::session::error::authentication_rejected);

}


int test_main(int, char*[])
{
    unit_test_init_session_success();
    unit_test_init_session_success_caps();
    unit_test_init_session_failure_authenticate();
    return 0;
}