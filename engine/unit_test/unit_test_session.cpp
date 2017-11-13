// Copyright (c) 2010-2015 Sami Väisänen, Ensisoft
//
// http://www.ensisoft.com
//
// This software is copyrighted software. Unauthorized hacking, cracking, distribution
// and general assing around is prohibited.
// Redistribution and use in source and binary forms, with or without modification,
// without permission are prohibited.
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "newsflash/config.h"
#include "newsflash/warnpush.h"
#  include <boost/test/minimal.hpp>
#include "newsflash/warnpop.h"

#include "engine/session.h"
#include "engine/buffer.h"

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
    nf::Session session;
    BOOST_REQUIRE(session.GetError() == nf::Session::Error::None);
    BOOST_REQUIRE(session.GetState() == nf::Session::State::None);

    std::string output;
    session.on_send = [&](const std::string& cmd) {
        output = cmd;
    };
    session.on_auth = [](std::string& user, std::string& pass) {
        user = "foo";
        pass = "bar";
    };

    nf::buffer incoming(1024);
    nf::buffer tmp(1);
    BOOST_REQUIRE(session.GetError() == nf::Session::Error::None);
    BOOST_REQUIRE(session.GetState() == nf::Session::State::None);

    session.Start();

    session.SendNext();
    BOOST_REQUIRE(output == "");
    set(incoming, "200 welcome posting allowed\r\n");
    session.RecvNext(incoming, tmp);

    session.SendNext();
    BOOST_REQUIRE(output == "CAPABILITIES\r\n");
    set(incoming, "500 what?\r\n");
    session.RecvNext(incoming, tmp);

    session.SendNext();
    BOOST_REQUIRE(output == "MODE READER\r\n");
    set(incoming, "480 authentication required\r\n");
    session.RecvNext(incoming, tmp);

    session.SendNext();
    BOOST_REQUIRE(output == "AUTHINFO USER foo\r\n");
    BOOST_REQUIRE(session.GetState() == nf::Session::State::Authenticate);
    set(incoming, "381 password required\r\n");
    session.RecvNext(incoming, tmp);

    session.SendNext();
    BOOST_REQUIRE(output == "AUTHINFO PASS bar\r\n");
    set(incoming, "281 authentication accepted\r\n");
    session.RecvNext(incoming, tmp);

    session.SendNext();
    BOOST_REQUIRE(output == "CAPABILITIES\r\n");
    //set(incoming, "500 what?\r\n");
    // this is technically not correct, but we make a little test case here in order
    // to workaround issue #29 (usenet.premiumize.me responds with 400 instead of 500 to CAPABILITIES)
    set(incoming, "400 Unrecognized command\r\n");
    session.RecvNext(incoming, tmp);

    session.SendNext();
    BOOST_REQUIRE(output == "MODE READER\r\n");
    set(incoming, "200 posting allowed\r\n");
    session.RecvNext(incoming, tmp);

    BOOST_REQUIRE(!session.HasPending());
    BOOST_REQUIRE(session.GetError() == nf::Session::Error::None);
    BOOST_REQUIRE(session.GetState() == nf::Session::State::Ready);
}

void unit_test_init_session_success_caps()
{
    nf::Session session;
    BOOST_REQUIRE(session.GetError() == nf::Session::Error::None);
    BOOST_REQUIRE(session.GetState() == nf::Session::State::None);

    std::string output;
    session.on_send = [&](const std::string& cmd) {
        output = cmd;
    };
    session.on_auth = [](std::string& user, std::string& pass) {
        user = "foo";
        pass = "bar";
    };

    nf::buffer incoming(1024);
    nf::buffer tmp(1);

    session.Start();

    session.SendNext();
    BOOST_REQUIRE(output == "");
    set(incoming, "200 welcome posting allowed\r\n");
    session.RecvNext(incoming, tmp);


    session.SendNext();
    BOOST_REQUIRE(output == "CAPABILITIES\r\n");
    set(incoming, "101 capabilities list follows\r\n"
        "MODE-READER\r\n"
        "XZVER\r\n"
        "IHAVE\r\n"
        "\r\n"
        ".\r\n");
    session.RecvNext(incoming, tmp);

    session.SendNext();
    BOOST_REQUIRE(output == "MODE READER\r\n");
    set(incoming, "200 posting allowed\r\n");
    session.RecvNext(incoming, tmp);

    BOOST_REQUIRE(!session.HasPending()); // done;
    BOOST_REQUIRE(session.HasGzipCompress() == false);
    BOOST_REQUIRE(session.HasXzver() == true);

}

void unit_test_init_session_garbage()
{
    nf::Session session;
    BOOST_REQUIRE(session.GetError() == nf::Session::Error::None);
    BOOST_REQUIRE(session.GetState() == nf::Session::State::None);

    session.on_send = [&](const std::string& cmd) {
    };
    session.on_auth = [](std::string& user, std::string& pass) {
        user = "foo";
        pass = "bar";
    };

    // junk on welcome
    {
        nf::buffer incoming(1024);
        nf::buffer tmp(1);

        session.Start();
        session.SendNext();

        set(incoming, "bla bhal bahsgaa\r\n");
        session.RecvNext(incoming, tmp);

        BOOST_REQUIRE(session.GetState() == nf::Session::State::Error);
        BOOST_REQUIRE(session.GetError() == nf::Session::Error::Protocol);
    }

    // junk (unexpected return value) command
    {
        nf::buffer incoming(1024);
        nf::buffer tmp(1);

        session.Start();
        session.SendNext();

        set(incoming, "999 welcome posting allowed\r\n");
        session.RecvNext(incoming, tmp);

        BOOST_REQUIRE(session.GetState() == nf::Session::State::Error);
        BOOST_REQUIRE(session.GetError() == nf::Session::Error::Protocol);
    }

}

void unit_test_init_session_failure_authenticate()
{
    nf::Session session;
    BOOST_REQUIRE(session.GetError() == nf::Session::Error::None);
    BOOST_REQUIRE(session.GetState() == nf::Session::State::None);

    std::string output;
    session.on_send = [&](const std::string& cmd) {
        output = cmd;
    };
    session.on_auth = [](std::string& user, std::string& pass) {
        user = "foo";
        pass = "bar";
    };

    nf::buffer incoming(1024);
    nf::buffer tmp(1);

    session.Start();

    session.SendNext();
    BOOST_REQUIRE(output == "");
    set(incoming, "200 welcome posting allowed\r\n");
    session.RecvNext(incoming, tmp);

    session.SendNext();
    BOOST_REQUIRE(output == "CAPABILITIES\r\n");
    set(incoming, "500 what?\r\n");
    session.RecvNext(incoming, tmp);

    session.SendNext();
    BOOST_REQUIRE(output == "MODE READER\r\n");
    set(incoming, "480 authentication required\r\n");
    session.RecvNext(incoming, tmp);

    session.SendNext();
    BOOST_REQUIRE(output == "AUTHINFO USER foo\r\n");
    set(incoming, "381 password required\r\n");
    session.RecvNext(incoming, tmp);

    session.SendNext();
    BOOST_REQUIRE(output == "AUTHINFO PASS bar\r\n");
    set(incoming, "482 authentication rejected\r\n");
    session.RecvNext(incoming, tmp);

    BOOST_REQUIRE(!session.HasPending());
    BOOST_REQUIRE(session.GetState() == nf::Session::State::Error);
    BOOST_REQUIRE(session.GetError() == nf::Session::Error::AuthenticationRejected);

}

void unit_test_change_group()
{
    nf::Session session;

    std::string output;
    session.on_send = [&](const std::string& cmd) {
        output = cmd;
    };

    nf::buffer incoming(1024);
    nf::buffer tmp(1024);

    session.ChangeGroup("alt.binaries.foo");
    session.SendNext();
    BOOST_REQUIRE(output == "GROUP alt.binaries.foo\r\n");
    set(incoming, "211 4 1 4 alt.binaries.foo group succesfully selected\r\n");
    session.RecvNext(incoming, tmp);

    BOOST_REQUIRE(!session.HasPending());
    BOOST_REQUIRE(session.GetState() == nf::Session::State::Ready);
    BOOST_REQUIRE(session.GetError() == nf::Session::Error::None);

    BOOST_REQUIRE(tmp.content_type() == nf::buffer::type::groupinfo);
    BOOST_REQUIRE(tmp.content_status() == nf::buffer::status::success);
    BOOST_REQUIRE(tmp.content_length() == std::strlen("211 4 1 4 alt.binaries.foo group succesfully selected\r\n"));
    BOOST_REQUIRE(!std::strncmp(tmp.content(),
        "211 4 1 4 alt.binaries.foo group succesfully selected\r\n", tmp.content_length()));
}

void unit_test_retrieve_article()
{
    nf::Session session;

    std::string output;
    session.on_send = [&](const std::string& cmd) {
        output = cmd;
    };

    // single command
    {
        nf::buffer incoming(1024);
        nf::buffer content(1024);

        session.RetrieveArticle("1234");
        session.SendNext();
        BOOST_REQUIRE(output == "BODY 1234\r\n");
        BOOST_REQUIRE(session.GetState() == nf::Session::State::Transfer);
        set(incoming, "222 body follows\r\n"
            "here's some text data\r\n"
            ".\r\n");
        session.RecvNext(incoming, content);
        BOOST_REQUIRE(!session.HasPending());
        BOOST_REQUIRE(session.GetState() == nf::Session::State::Ready);
        BOOST_REQUIRE(session.GetError() == nf::Session::Error::None);

        BOOST_REQUIRE(content.content_type() == nf::buffer::type::article);
        BOOST_REQUIRE(content.content_start() == std::strlen("222 body follows\r\n"));
        BOOST_REQUIRE(content.content_length() == std::strlen("here's some text data\r\n.\r\n"));
        BOOST_REQUIRE(!std::strncmp(content.content(),
            "here's some text data\r\n.\r\n", content.content_length()));
    }

    // multiple enqueued commands, no pipelining
    {
        nf::buffer incoming(1024);
        nf::buffer content(1024);

        session.RetrieveArticle("1234");
        session.RetrieveArticle("2345");
        session.RetrieveArticle("3456");

        session.SendNext();
        BOOST_REQUIRE(output == "BODY 1234\r\n");
        set(incoming, "222 body follows\r\nthis is first content\r\n.\r\n");
        session.RecvNext(incoming, content);
        BOOST_REQUIRE(content.content_type() == nf::buffer::type::article);
        BOOST_REQUIRE(content.content_status() == nf::buffer::status::success);
        BOOST_REQUIRE(!std::strncmp(content.content(),
            "this is first content\r\n.\r\n", content.content_length()));

        session.SendNext();
        BOOST_REQUIRE(output == "BODY 2345\r\n");
        set(incoming, "420 no article with that message id dmca takedown\r\n");
        session.RecvNext(incoming, content);
        BOOST_REQUIRE(content.content_type() == nf::buffer::type::article);
        BOOST_REQUIRE(content.content_status() == nf::buffer::status::dmca);

        session.SendNext();
        BOOST_REQUIRE(output == "BODY 3456\r\n");
        set(incoming, "423 no article with that number\r\n");
        session.RecvNext(incoming, content);
        BOOST_REQUIRE(content.content_type() == nf::buffer::type::article);
        BOOST_REQUIRE(content.content_status() == nf::buffer::status::unavailable);

        BOOST_REQUIRE(!session.HasPending());
    }

    // pipelining
    {
        nf::buffer incoming(1024);
        nf::buffer content(1024);


        session.on_send = [&](const std::string& cmd) {
            output.append(cmd);
            output.append("/");
        };

        output.clear();

        session.SetEnablePipelining(true);
        session.RetrieveArticle("1234");
        session.RetrieveArticle("2345");
        session.RetrieveArticle("3456");

        session.SendNext();

        BOOST_REQUIRE(output == "BODY 1234\r\n/BODY 2345\r\n/BODY 3456\r\n/");
        set(incoming, "222 body follows\r\nthis is first content\r\n.\r\n"
            "222 body fol");

        BOOST_REQUIRE(session.RecvNext(incoming, content));
        BOOST_REQUIRE(content.content_type() == nf::buffer::type::article);
        BOOST_REQUIRE(!std::strncmp(content.content(),
            "this is first content\r\n.\r\n", content.content_length()));

        append(incoming, "lows\r\nsecond content\r\n.\r\n"
            "423 no such article with that number\r\n");

        BOOST_REQUIRE(session.RecvNext(incoming, content));
        BOOST_REQUIRE(!std::strncmp(content.content(),
            "second content\r\n.\r\n", content.content_length()));

        BOOST_REQUIRE(session.RecvNext(incoming, content));
        BOOST_REQUIRE(content.content_status() == nf::buffer::status::unavailable);

        BOOST_REQUIRE(!session.HasPending());
    }

}

void unit_test_retrieve_listing()
{
    nf::Session session;

    std::string output;
    session.on_send = [&](const std::string& cmd) {
        output = cmd;
    };

    nf::buffer i(1024);
    nf::buffer o(1024);

    session.RetrieveList();
    session.SendNext();
    BOOST_REQUIRE(output == "LIST\r\n");

    set(i, "215 listing follows\r\n"
        "alt.binaries.foo 1 0 y\r\n"
        "alt.binaries.bar 2 1 n\r\n"
        ".\r\n");
    session.RecvNext(i, o);

    BOOST_REQUIRE(session.HasPending() == false);
    BOOST_REQUIRE(o.content_status() == nf::buffer::status::success);
    BOOST_REQUIRE(o.content_type() == nf::buffer::type::grouplist);
    BOOST_REQUIRE(o.content_start() == strlen("215 listing follows\r\n"));
    BOOST_REQUIRE(o.content_length() == strlen("alt.binaries.foo 1 0 y\r\n"
        "alt.binaries.bar 2 1 n\r\n.\r\n"));
}

int test_main(int, char*[])
{
    unit_test_init_session_success();
    unit_test_init_session_success_caps();
    unit_test_init_session_garbage();
    unit_test_init_session_failure_authenticate();
    unit_test_change_group();
    unit_test_retrieve_article();
    unit_test_retrieve_listing();
    return 0;
}