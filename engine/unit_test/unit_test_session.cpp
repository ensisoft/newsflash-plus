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

void set(nf::Buffer& buff, const char* str)
{
    buff.Clear();
    std::strcpy(buff.Back(), str);
    buff.Append(std::strlen(str));
}

void append(nf::Buffer& buff, const char* str)
{
    std::strcpy(buff.Back(), str);
    buff.Append(std::strlen(str));
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

    nf::Buffer incoming(1024);
    nf::Buffer tmp(1);
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

    nf::Buffer incoming(1024);
    nf::Buffer tmp(1);

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

    nf::Buffer incoming(1024);
    nf::Buffer tmp(1);

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

    nf::Buffer incoming(1024);
    nf::Buffer tmp(1024);

    session.ChangeGroup("alt.binaries.foo");
    session.SendNext();
    BOOST_REQUIRE(output == "GROUP alt.binaries.foo\r\n");
    set(incoming, "211 4 1 4 alt.binaries.foo group succesfully selected\r\n");
    session.RecvNext(incoming, tmp);

    BOOST_REQUIRE(!session.HasPending());
    BOOST_REQUIRE(session.GetState() == nf::Session::State::Ready);
    BOOST_REQUIRE(session.GetError() == nf::Session::Error::None);

    BOOST_REQUIRE(tmp.GetContentType() == nf::Buffer::Type::GroupInfo);
    BOOST_REQUIRE(tmp.GetContentStatus() == nf::Buffer::Status::Success);
    BOOST_REQUIRE(tmp.GetContentLength() == std::strlen("211 4 1 4 alt.binaries.foo group succesfully selected\r\n"));
    BOOST_REQUIRE(!std::strncmp(tmp.Content(),
        "211 4 1 4 alt.binaries.foo group succesfully selected\r\n", tmp.GetContentLength()));
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
        nf::Buffer incoming(1024);
        nf::Buffer content(1024);

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

        BOOST_REQUIRE(content.GetContentType() == nf::Buffer::Type::Article);
        BOOST_REQUIRE(content.GetContentStart() == std::strlen("222 body follows\r\n"));
        BOOST_REQUIRE(content.GetContentLength() == std::strlen("here's some text data\r\n.\r\n"));
        BOOST_REQUIRE(!std::strncmp(content.Content(),
            "here's some text data\r\n.\r\n", content.GetContentLength()));
    }

    // multiple enqueued commands, no pipelining
    {
        nf::Buffer incoming(1024);
        nf::Buffer content(1024);

        session.RetrieveArticle("1234");
        session.RetrieveArticle("2345");
        session.RetrieveArticle("3456");

        session.SendNext();
        BOOST_REQUIRE(output == "BODY 1234\r\n");
        set(incoming, "222 body follows\r\nthis is first content\r\n.\r\n");
        session.RecvNext(incoming, content);
        BOOST_REQUIRE(content.GetContentType() == nf::Buffer::Type::Article);
        BOOST_REQUIRE(content.GetContentStatus() == nf::Buffer::Status::Success);
        BOOST_REQUIRE(!std::strncmp(content.Content(),
            "this is first content\r\n.\r\n", content.GetContentLength()));

        session.SendNext();
        BOOST_REQUIRE(output == "BODY 2345\r\n");
        set(incoming, "420 no article with that message id dmca takedown\r\n");
        session.RecvNext(incoming, content);
        BOOST_REQUIRE(content.GetContentType() == nf::Buffer::Type::Article);
        BOOST_REQUIRE(content.GetContentStatus() == nf::Buffer::Status::Dmca);

        session.SendNext();
        BOOST_REQUIRE(output == "BODY 3456\r\n");
        set(incoming, "423 no article with that number\r\n");
        session.RecvNext(incoming, content);
        BOOST_REQUIRE(content.GetContentType() == nf::Buffer::Type::Article);
        BOOST_REQUIRE(content.GetContentStatus() == nf::Buffer::Status::Unavailable);

        BOOST_REQUIRE(!session.HasPending());
    }

    // pipelining
    {
        nf::Buffer incoming(1024);
        nf::Buffer content(1024);


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
        BOOST_REQUIRE(content.GetContentType() == nf::Buffer::Type::Article);
        BOOST_REQUIRE(!std::strncmp(content.Content(),
            "this is first content\r\n.\r\n", content.GetContentLength()));

        append(incoming, "lows\r\nsecond content\r\n.\r\n"
            "423 no such article with that number\r\n");

        BOOST_REQUIRE(session.RecvNext(incoming, content));
        BOOST_REQUIRE(!std::strncmp(content.Content(),
            "second content\r\n.\r\n", content.GetContentLength()));

        BOOST_REQUIRE(session.RecvNext(incoming, content));
        BOOST_REQUIRE(content.GetContentStatus() == nf::Buffer::Status::Unavailable);

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

    nf::Buffer i(1024);
    nf::Buffer o(1024);

    session.RetrieveList();
    session.SendNext();
    BOOST_REQUIRE(output == "LIST\r\n");

    set(i, "215 listing follows\r\n"
        "alt.binaries.foo 1 0 y\r\n"
        "alt.binaries.bar 2 1 n\r\n"
        ".\r\n");
    session.RecvNext(i, o);

    BOOST_REQUIRE(session.HasPending() == false);
    BOOST_REQUIRE(o.GetContentStatus() == nf::Buffer::Status::Success);
    BOOST_REQUIRE(o.GetContentType() == nf::Buffer::Type::GroupList);
    BOOST_REQUIRE(o.GetContentStart() == strlen("215 listing follows\r\n"));
    BOOST_REQUIRE(o.GetContentLength() == strlen("alt.binaries.foo 1 0 y\r\n"
        "alt.binaries.bar 2 1 n\r\n.\r\n"));
}

void unit_test_unexpected_response()
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
        nf::Buffer incoming(1024);
        nf::Buffer tmp(1);

        session.Reset();
        session.Start();
        session.SendNext();

        set(incoming, "bla bhal bahsgaa\r\n");
        session.RecvNext(incoming, tmp);

        BOOST_REQUIRE(session.GetState() == nf::Session::State::Error);
        BOOST_REQUIRE(session.GetError() == nf::Session::Error::Protocol);
    }

    // junk (unexpected return value) command
    {
        nf::Buffer incoming(1024);
        nf::Buffer tmp(1);

        session.Reset();
        session.Start();
        session.SendNext();

        set(incoming, "999 welcome posting allowed\r\n");
        session.RecvNext(incoming, tmp);

        BOOST_REQUIRE(session.GetState() == nf::Session::State::Error);
        BOOST_REQUIRE(session.GetError() == nf::Session::Error::Protocol);
    }

    // authentication during body command
    // current expectation is that the authentication should not happen during
    // the BODY command, thus rendering a protcol error.
    {
        nf::Buffer incoming(1024);
        nf::Buffer tmp(1);

        session.Reset();
        session.Start();
        session.SendNext();
        set(incoming, "200 welcome posting allowed\r\n");
        session.RecvNext(incoming, tmp);

        session.SendNext();
        set(incoming, "101 capabilities list follows\r\n"
            "MODE-READER\r\n"
            "XZVER\r\n"
            "IHAVE\r\n"
            "\r\n"
            ".\r\n");
        session.RecvNext(incoming, tmp);

        session.SendNext();
        set(incoming, "200 posting allowed\r\n");
        session.RecvNext(incoming, tmp);

        session.RetrieveArticle("<blah>");
        session.SendNext();
        set(incoming, "480 authentication required\r\n");
        session.RecvNext(incoming, tmp);

        BOOST_REQUIRE(session.GetState() == nf::Session::State::Error);
        BOOST_REQUIRE(session.GetError() == nf::Session::Error::Protocol);
    }
}

int test_main(int, char*[])
{
    unit_test_init_session_success();
    unit_test_init_session_success_caps();
    unit_test_init_session_failure_authenticate();
    unit_test_change_group();
    unit_test_retrieve_article();
    unit_test_retrieve_listing();

    unit_test_unexpected_response();
    return 0;
}