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

#include <thread>
#include <deque>
#include <string>
#include <fstream>
#include <iterator>

#include "engine/connection.h"
#include "engine/action.h"
#include "engine/sockets.h"
#include "engine/socketapi.h"
#include "engine/cmdlist.h"
#include "engine/session.h"
#include "engine/buffer.h"
#include "engine/logging.h"
#include "engine/decode.h"
#include "engine/throttle.h"
#include "unit_test_common.h"

namespace nf = newsflash;

void test_initial_state()
{
    nf::connection conn;
    BOOST_REQUIRE(conn.num_bytes_transferred() == 0);
    BOOST_REQUIRE(conn.current_speed_bps() == 0);
    BOOST_REQUIRE(conn.get_state() == nf::connection::state::disconnected);
    BOOST_REQUIRE(conn.get_error() == nf::connection::error::none);
}

void test_connect()
{
    auto log = std::make_shared<nf::stdlog>(std::cout);

    std::unique_ptr<nf::action> act;

    // resolve fails
    {
        nf::connection::spec s;
        s.hostname = "foobar";
        s.hostport = 1818;
        s.use_ssl  = false;

        nf::connection conn;

        act = conn.connect(s);
        act->set_log(log);
        act->perform();
        BOOST_REQUIRE(conn.get_state() == nf::connection::state::resolving);
        act = conn.complete(std::move(act));
        BOOST_REQUIRE(conn.get_state() == nf::connection::state::error);
        BOOST_REQUIRE(conn.get_error() == nf::connection::error::resolve);
    }

    // connect fails
    {
        nf::connection::spec s;
        s.hostname = "localhost";
        s.hostport = 1818;
        s.use_ssl  = false;

        nf::connection conn;

        // resolve
        act = conn.connect(s);
        act->set_log(log);
        act->perform();

        // connect
        act = conn.complete(std::move(act));
        BOOST_REQUIRE(conn.get_state() == nf::connection::state::connecting);
        BOOST_REQUIRE(conn.get_error() == nf::connection::error::none);
        act->set_log(log);
        act->perform();

        // initialize
        act = conn.complete(std::move(act));
        BOOST_REQUIRE(conn.get_state() == nf::connection::state::error);
        BOOST_REQUIRE(conn.get_error() == nf::connection::error::refused);
    }

    // nntp init fails
    {

        nf::connection::spec s;
        s.hostname = "www.google.com";
        s.hostport = 80;
        s.use_ssl  = false;

        nf::connection conn;

        // resolve
        act = conn.connect(s);
        act->set_log(log);
        act->perform(); // resolve
        act = conn.complete(std::move(act));

        // connect
        act->set_log(log);
        act->perform(); // connect
        act = conn.complete(std::move(act));

        // initialize
        act->set_log(log);
        act->perform();
        BOOST_REQUIRE(conn.get_state() == nf::connection::state::initializing);
        BOOST_REQUIRE(conn.get_error() == nf::connection::error::none);

        act = conn.complete(std::move(act));
        BOOST_REQUIRE(conn.get_state() == nf::connection::state::error);
        BOOST_REQUIRE(conn.get_error() == nf::connection::error::timeout);
    }

    // authentication fails (no such user at the server)
    {
        nf::connection::spec s;
        s.hostname = "localhost";
        s.hostport = 1919;
        s.use_ssl  = false;
        s.username = "fail";
        s.password = "fail";
        s.enable_compression = false;
        s.enable_pipelining = false;

        nf::connection conn;

        // resolve
        act = conn.connect(s);
        act->set_log(log);
        act->perform();
        act = conn.complete(std::move(act));

        // connect
        act->set_log(log);
        act->perform();
        act = conn.complete(std::move(act));

        // initialize
        act->set_log(log);
        act->perform();
        BOOST_REQUIRE(conn.get_state() == nf::connection::state::initializing);
        BOOST_REQUIRE(conn.get_error() == nf::connection::error::none);

        act = conn.complete(std::move(act));
        BOOST_REQUIRE(conn.get_state() == nf::connection::state::error);
        BOOST_REQUIRE(conn.get_error() == nf::connection::error::authentication_rejected);

    }

    // no permission (user exists, but out of quota/account locked etc)
    {
        // todo:
    }

    // succesful connect
    {
        nf::connection::spec s;
        s.hostname = "localhost";
        s.hostport = 1919;
        s.use_ssl  = false;
        s.username = "pass";
        s.password = "pass";
        s.enable_compression = false;
        s.enable_pipelining  = false;
        s.pthrottle = nullptr;

        nf::connection conn;

        // resolve
        act = conn.connect(s);
        act->set_log(log);
        act->perform();
        act = conn.complete(std::move(act)); // resolve

        // connect
        act->set_log(log);
        act->perform();
        act = conn.complete(std::move(act));

        // initialize
        act->set_log(log);
        act->perform();
        act = conn.complete(std::move(act));
        BOOST_REQUIRE(conn.get_state() == nf::connection::state::connected);
        BOOST_REQUIRE(conn.get_error() == nf::connection::error::none);
    }

    // disconnect
    {
        nf::connection::spec s;
        s.hostname = "localhost";
        s.hostport     = 1919;
        s.use_ssl  = false;
        s.username = "pass";
        s.password = "pass";
        s.enable_pipelining = false;
        s.enable_compression = false;
        s.pthrottle = nullptr;

        nf::connection conn;

        // resolve
        act = conn.connect(s);
        act->set_log(log);
        act->perform();
        act = conn.complete(std::move(act));

        // connect
        act->set_log(log);
        act->perform();
        act = conn.complete(std::move(act));

        // initialize
        act->set_log(log);
        act->perform();
        act = conn.complete(std::move(act));

        // disconnect
        act = conn.disconnect();
        act->set_log(log);
        act->perform();
        act = conn.complete(std::move(act));
        BOOST_REQUIRE(conn.get_state() == nf::connection::state::disconnected);
        BOOST_REQUIRE(conn.get_error() == nf::connection::error::none);
    }

    // discard the connection object while connecting
    {
        nf::connection::spec s;
        s.hostname = "localhost";
        s.hostport = 1919;
        s.use_ssl  = false;
        s.username = "pass";
        s.password = "pass";
        s.enable_pipelining = false;
        s.enable_compression = false;
        s.pthrottle = nullptr;

        std::unique_ptr<nf::connection> conn(new nf::connection);

        // resolve.
        act = conn->connect(s);
        act->set_log(log);
        act->perform();
        act = conn->complete(std::move(act));

        act->set_log(log);
        // for the connect action,
        // spawn a new thread that executes the action the background
        // meanwhile the main thread destroys the object.
        std::thread thread([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            act->perform();
        });

        conn.reset();
        thread.join();
    }

    nf::set_thread_log(nullptr);
}

void test_execute_success()
{
    auto log = std::make_shared<nf::stdlog>(std::cout);

    std::unique_ptr<nf::action> act;

    nf::throttle throttle;

    nf::connection::spec s;
    s.hostname = "localhost";
    s.hostport = 1919;
    s.use_ssl  = false;
    s.enable_compression = false;
    s.enable_pipelining  = false;
    s.username  = "pass";
    s.password  = "pass";
    s.pthrottle = &throttle;

    nf::connection::cmdlist_completion_data completion;

    nf::connection conn;
    conn.set_callback([&](const nf::connection::cmdlist_completion_data& data) {
        completion = data;
    });

    // resolve
    act = conn.connect(s);
    act->set_log(log);
    act->perform();

    // connect
    act = conn.complete(std::move(act));
    act->set_log(log);
    act->perform();

    // initialize
    act = conn.complete(std::move(act));
    act->set_log(log);
    act->perform();

    // successful body retrieval
    {
        nf::CmdList::Messages m;
        m.groups  = {"alt.binaries.foo"};
        m.numbers = {"3"};

        auto cmds = std::make_shared<nf::CmdList>(m);

        // execute
        act = conn.execute(cmds, 123);
        BOOST_REQUIRE(conn.get_state() == nf::connection::state::active);
        act->perform();
        act->run_completion_callbacks();
        act = conn.complete(std::move(act));
        BOOST_REQUIRE(conn.get_state() == nf::connection::state::connected);
        BOOST_REQUIRE(conn.get_error() == nf::connection::error::none);
        BOOST_REQUIRE(!act);

        BOOST_REQUIRE(completion.success);
        BOOST_REQUIRE(completion.cmds == cmds);
        BOOST_REQUIRE(completion.task_owner_id == 123);
        BOOST_REQUIRE(completion.total_bytes != 0); // todo:
        BOOST_REQUIRE(completion.content_bytes != 0); // todo:

        BOOST_REQUIRE(cmds->NumBuffers() == 1);
        BOOST_REQUIRE(cmds->GetBuffer(0).content_type() == nf::buffer::type::article);
        BOOST_REQUIRE(cmds->GetBuffer(0).content_status()== nf::buffer::status::success);
    }

    // no such body
    {
        nf::CmdList::Messages m;
        m.groups  = {"alt.binaries.foo"};
        m.numbers = {"1"};

        auto cmds = std::make_shared<nf::CmdList>(m);

        act = conn.execute(cmds, 123);
        act->perform();
        act->run_completion_callbacks();
        act = conn.complete(std::move(act));
        BOOST_REQUIRE(conn.get_state() == nf::connection::state::connected);
        BOOST_REQUIRE(conn.get_error() == nf::connection::error::none);

        BOOST_REQUIRE(completion.success);
        BOOST_REQUIRE(completion.cmds == cmds);
        BOOST_REQUIRE(completion.task_owner_id == 123);
        BOOST_REQUIRE(completion.total_bytes != 0); // todo:
        BOOST_REQUIRE(completion.content_bytes == 0);

        BOOST_REQUIRE(cmds->NumBuffers() == 1);
        BOOST_REQUIRE(cmds->GetBuffer(0).content_type() == nf::buffer::type::article);
        BOOST_REQUIRE(cmds->GetBuffer(0).content_status()== nf::buffer::status::unavailable);

    }
}

void test_execute_failure()
{
    // todo:
}

void test_cancel_execute()
{
    // todo:
}



int test_main(int argc, char* argv[])
{
    test_initial_state();
    test_connect();
    test_execute_success();
    test_execute_failure();
    test_cancel_execute();
    return 0;
}
