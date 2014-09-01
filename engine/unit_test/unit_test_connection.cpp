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
#include <thread>
#include <deque>
#include <string>
#include <fstream>
#include <iterator>
#include "../connection.h"
#include "../action.h"
#include "../sockets.h"
#include "../socketapi.h"
#include "../logging.h"
#include "../cmdlist.h"
#include "../session.h"
#include "../buffer.h"

namespace nf = newsflash;

#define REQUIRE_EXCEPTION(x) \
    try { \
        x; \
        BOOST_FAIL("exception was expected"); \
    } catch(const std::exception& e) {}


void test_connect()
{
    std::unique_ptr<nf::action> act;

    nf::connection conn(1, 1);
    conn.on_action = [&](std::unique_ptr<nf::action> a) {
        act = std::move(a);
    };

    using state = nf::connection::state;
    using error = nf::connection::error;

    BOOST_REQUIRE(conn.get_state() == state::disconnected);

    LOG_OPEN("test", "test_connect.log");

    // resolve fails
    {
        nf::connection::server serv;
        serv.hostname = "foobar";
        serv.port     = 1818;
        conn.connect(serv);
        BOOST_REQUIRE(conn.get_state() == state::resolving);
        act->perform();
        BOOST_REQUIRE(act->has_exception());
        conn.complete(std::move(act));
        BOOST_REQUIRE(conn.get_state() == state::error);
        BOOST_REQUIRE(conn.get_error() == error::resolve);

        LOG_D("end");
        LOG_FLUSH();
    }

    // connect fails
    {
        nf::connection::server serv;
        serv.hostname = "localhost";
        serv.port     = 1818;
        serv.use_ssl  = false;
        conn.connect(serv);
        act->perform();
        BOOST_REQUIRE(!act->has_exception());

        conn.complete(std::move(act));
        BOOST_REQUIRE(conn.get_state() == state::connecting);
        act->perform();
        BOOST_REQUIRE(act->has_exception());
        conn.complete(std::move(act));
        BOOST_REQUIRE(conn.get_state() == state::error);
        BOOST_REQUIRE(conn.get_error() == error::refused);

        LOG_D("end");        
        LOG_FLUSH();        
    }

    // nntp init fails
    // {
    //     //auto sp = open_server();

    //     nf::connection::server serv;
    //     serv.hostname = "localhost";
    //     serv.port     = 1919;
    //     serv.use_ssl  = false;
    //     conn.connect(serv);
    //     act->perform(); // resolve
    //     conn.complete(std::move(act));

    //     act->perform(); // connect
    //     conn.complete(std::move(act));
    //     BOOST_REQUIRE(conn.get_state() == state::initializing);
    //     act->perform(); // initialize
    //     BOOST_REQUIRE(act->has_exception());
    //     conn.complete(std::move(act));
    //     BOOST_REQUIRE(conn.get_state() == state::error);
    //     BOOST_REQUIRE(conn.get_error() == error::other);

    //     LOG_D("end");        
    //     LOG_FLUSH();
    // }

    // authentication fails
    {

        nf::connection::server serv;
        serv.hostname = "localhost";
        serv.port     = 1919;
        serv.use_ssl  = false;
        serv.username = "fail";
        serv.password = "fail";
        conn.connect(serv); 
        act->perform(); // resolve
        conn.complete(std::move(act));

        act->perform(); // connect
        conn.complete(std::move(act));
        act->perform(); // initialize
        conn.complete(std::move(act));
        BOOST_REQUIRE(conn.get_state() == state::error);
        BOOST_REQUIRE(conn.get_error() == error::authentication_rejected);

        LOG_D("end");        
        LOG_FLUSH();            
    }

    // cancel
    {
        nf::connection::server serv;
        serv.hostname = "localhost";
        serv.port     = 1919;
        serv.use_ssl  = false;
        serv.username = "pass";
        serv.password = "pass";
        conn.connect(serv);

        act->perform(); // resolve
        conn.complete(std::move(act));

        conn.cancel();
        act->perform(); // connect
        conn.complete(std::move(act));

        BOOST_REQUIRE(conn.get_state() == state::disconnected);
        BOOST_REQUIRE(conn.get_error() == error::none);
    }

    // succesful connect
    {
        nf::connection::server serv;
        serv.hostname = "localhost";
        serv.port = 1919;
        serv.use_ssl = false;
        serv.username = "pass";
        serv.password = "pass";
        conn.connect(serv);

        act->perform();
        conn.complete(std::move(act)); // resolve

        act->perform(); 
        conn.complete(std::move(act)); // connect

        act->perform();
        conn.complete(std::move(act)); // initialize

        BOOST_REQUIRE(conn.get_state() == state::connected);
        BOOST_REQUIRE(conn.get_error() == error::none);
    }
}

void test_execute()
{
    std::unique_ptr<nf::action> act;

    nf::connection conn(1, 1);
    conn.on_action = [&](std::unique_ptr<nf::action> a) {
        act = std::move(a);
    };

    using state = nf::connection::state;
    using error = nf::connection::error;

    nf::connection::server serv;
    serv.hostname = "localhost";
    serv.port = 1919;
    serv.use_ssl = false;
    serv.username = "pass";
    serv.password = "pass";
    conn.connect(serv);

    act->perform(); // resolve
    conn.complete(std::move(act));

    act->perform(); // connect
    conn.complete(std::move(act));

    act->perform(); // initialize
    conn.complete(std::move(act));

    BOOST_REQUIRE(conn.get_state() == state::connected);
    BOOST_REQUIRE(conn.get_error() == error::none);

    class cmdlist : public nf::cmdlist 
    {
    public:
        cmdlist() : buffers_(0), content_len_(0)
        {}

        virtual bool is_done(nf::cmdlist::step step) const  override
        {
            if (step == cmdlist::step::configure)
                return true;

            return buffers_ == 2;
        }

        virtual void submit(nf::cmdlist::step step, nf::session& session) override
        {
            if (step == cmdlist::step::configure)
                return;

            // see server.cpp
            session.retrieve_article("1");
            session.retrieve_article("4");
        }

        virtual void receive(nf::cmdlist::step, nf::buffer& buffer) override
        {
            if (buffers_ == 0)
            {
                BOOST_REQUIRE(buffer.content_status() == nf::buffer::status::unavailable);
            }
            else if (buffers_ == 1)
            {
                // note the dot doublings in the content
                std::string content = 
                    "here is some textual content\r\n"
                    "first line.\r\n"
                    ".. second line starts with a dot\r\n"
                    "... two dots\r\n"
                    "foo . bar\r\n"
                    "....\r\n"
                    "end\r\n"
                    ".\r\n";
                BOOST_REQUIRE(buffer.content_status() == nf::buffer::status::success);
                BOOST_REQUIRE(buffer.content_length() == content.size());
                BOOST_REQUIRE(!std::strncmp(buffer.content(), content.c_str(),
                    content.size()));
                content_len_ = content.size();
            }
            buffers_++;
        }

        virtual void next(nf::cmdlist::step) 
        {}
    public:
        int buffers_;
        int content_len_;
    };

    cmdlist test;

    conn.execute("test", 123, &test);
    BOOST_REQUIRE(conn.get_state() == state::active);

    auto ui = conn.get_ui_state();
    BOOST_REQUIRE(ui.desc == "test");
    BOOST_REQUIRE(ui.task == 123);

    act->perform(); // execute cmdlist
    conn.complete(std::move(act));

    BOOST_REQUIRE(conn.get_state() == state::connected);
    BOOST_REQUIRE(conn.get_error() == error::none);

    ui = conn.get_ui_state();
    BOOST_REQUIRE(ui.desc == "");
    BOOST_REQUIRE(ui.task == 0);
    BOOST_REQUIRE(ui.down >= test.content_len_);
}

void test_cancel_execute()
{
    // todo:
}

int test_main(int argc, char* argv[])
{
    test_connect();
    test_execute();
    test_cancel_execute();

    return 0;
}
