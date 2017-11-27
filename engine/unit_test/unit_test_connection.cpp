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
    nf::ConnectionImpl conn;
    BOOST_REQUIRE(conn.GetNumBytesTransferred() == 0);
    BOOST_REQUIRE(conn.GetCurrentSpeedBps() == 0);
    BOOST_REQUIRE(conn.GetState() == nf::Connection::State::Disconnected);
    BOOST_REQUIRE(conn.GetError() == nf::Connection::Error::None);
}

void test_connect()
{
    auto log = std::make_shared<nf::StdLogger>(std::cout);

    std::unique_ptr<nf::action> act;

    // resolve fails
    {
        nf::Connection::HostDetails s;
        s.hostname = "foobar";
        s.hostport = 1818;
        s.use_ssl  = false;

        nf::ConnectionImpl conn;

        act = conn.Connect(s);
        act->set_log(log);
        act->perform();
        BOOST_REQUIRE(conn.GetState() == nf::Connection::State::Resolving);
        act = conn.Complete(std::move(act));
        BOOST_REQUIRE(conn.GetState() == nf::Connection::State::Error);
        BOOST_REQUIRE(conn.GetError() == nf::Connection::Error::Resolve);
    }

    // Connect fails
    {
        nf::Connection::HostDetails s;
        s.hostname = "localhost";
        s.hostport = 1818;
        s.use_ssl  = false;

        nf::ConnectionImpl conn;

        // resolve
        act = conn.Connect(s);
        act->set_log(log);
        act->perform();

        // Connect
        act = conn.Complete(std::move(act));
        BOOST_REQUIRE(conn.GetState() == nf::Connection::State::Connecting);
        BOOST_REQUIRE(conn.GetError() == nf::Connection::Error::None);
        act->set_log(log);
        act->perform();

        // initialize
        act = conn.Complete(std::move(act));
        BOOST_REQUIRE(conn.GetState() == nf::Connection::State::Error);
        BOOST_REQUIRE(conn.GetError() == nf::Connection::Error::Refused);
    }

    // nntp init fails
    {

        nf::Connection::HostDetails s;
        s.hostname = "www.google.com";
        s.hostport = 80;
        s.use_ssl  = false;

        nf::ConnectionImpl conn;

        // resolve
        act = conn.Connect(s);
        act->set_log(log);
        act->perform(); // resolve
        act = conn.Complete(std::move(act));

        // Connect
        act->set_log(log);
        act->perform(); // Connect
        act = conn.Complete(std::move(act));

        // initialize
        act->set_log(log);
        act->perform();
        BOOST_REQUIRE(conn.GetState() == nf::Connection::State::Initializing);
        BOOST_REQUIRE(conn.GetError() == nf::Connection::Error::None);

        act = conn.Complete(std::move(act));
        BOOST_REQUIRE(conn.GetState() == nf::Connection::State::Error);
        BOOST_REQUIRE(conn.GetError() == nf::Connection::Error::Timeout);
    }

    // authentication fails (no such user at the server)
    {
        nf::Connection::HostDetails s;
        s.hostname = "localhost";
        s.hostport = 1919;
        s.use_ssl  = false;
        s.username = "fail";
        s.password = "fail";
        s.enable_compression = false;
        s.enable_pipelining = false;

        nf::ConnectionImpl conn;

        // resolve
        act = conn.Connect(s);
        act->set_log(log);
        act->perform();
        act = conn.Complete(std::move(act));

        // Connect
        act->set_log(log);
        act->perform();
        act = conn.Complete(std::move(act));

        // initialize
        act->set_log(log);
        act->perform();
        BOOST_REQUIRE(conn.GetState() == nf::Connection::State::Initializing);
        BOOST_REQUIRE(conn.GetError() == nf::Connection::Error::None);

        act = conn.Complete(std::move(act));
        BOOST_REQUIRE(conn.GetState() == nf::Connection::State::Error);
        BOOST_REQUIRE(conn.GetError() == nf::Connection::Error::AuthenticationRejected);

    }

    // no permission (user exists, but out of quota/account locked etc)
    {
        // todo:
    }
    // succesful Connect
    {
        nf::Connection::HostDetails s;
        s.hostname = "localhost";
        s.hostport = 1919;
        s.use_ssl  = false;
        s.username = "pass";
        s.password = "pass";
        s.enable_compression = false;
        s.enable_pipelining  = false;
        s.pthrottle = nullptr;

        nf::ConnectionImpl conn;

        // resolve
        act = conn.Connect(s);
        act->set_log(log);
        act->perform();
        act = conn.Complete(std::move(act)); // resolve

        // Connect
        act->set_log(log);
        act->perform();
        act = conn.Complete(std::move(act));

        // initialize
        act->set_log(log);
        act->perform();
        act = conn.Complete(std::move(act));
        BOOST_REQUIRE(conn.GetState() == nf::Connection::State::Connected);
        BOOST_REQUIRE(conn.GetError() == nf::Connection::Error::None);
    }

    // disconnect
    {
        nf::Connection::HostDetails s;
        s.hostname = "localhost";
        s.hostport     = 1919;
        s.use_ssl  = false;
        s.username = "pass";
        s.password = "pass";
        s.enable_pipelining = false;
        s.enable_compression = false;
        s.pthrottle = nullptr;

        nf::ConnectionImpl conn;

        // resolve
        act = conn.Connect(s);
        act->set_log(log);
        act->perform();
        act = conn.Complete(std::move(act));

        // Connect
        act->set_log(log);
        act->perform();
        act = conn.Complete(std::move(act));

        // initialize
        act->set_log(log);
        act->perform();
        act = conn.Complete(std::move(act));

        // disconnect
        act = conn.Disconnect();
        act->set_log(log);
        act->perform();
        act = conn.Complete(std::move(act));
        BOOST_REQUIRE(conn.GetState() == nf::Connection::State::Disconnected);
        BOOST_REQUIRE(conn.GetError() == nf::Connection::Error::None);
    }

    // discard the Connection object while connecting
    {
        nf::Connection::HostDetails s;
        s.hostname = "localhost";
        s.hostport = 1919;
        s.use_ssl  = false;
        s.username = "pass";
        s.password = "pass";
        s.enable_pipelining = false;
        s.enable_compression = false;
        s.pthrottle = nullptr;

        std::unique_ptr<nf::Connection> conn(new nf::ConnectionImpl);

        // resolve.
        act = conn->Connect(s);
        act->set_log(log);
        act->perform();
        act = conn->Complete(std::move(act));

        act->set_log(log);
        // for the Connect action,
        // spawn a new thread that executes the action the background
        // meanwhile the main thread destroys the object.
        std::thread thread([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            act->perform();
        });

        conn.reset();
        thread.join();
    }

    nf::SetThreadLog(nullptr);
}

void test_execute_success()
{
    auto log = std::make_shared<nf::StdLogger>(std::cout);

    std::unique_ptr<nf::action> act;

    nf::throttle throttle;

    nf::Connection::HostDetails s;
    s.hostname = "localhost";
    s.hostport = 1919;
    s.use_ssl  = false;
    s.enable_compression = false;
    s.enable_pipelining  = false;
    s.username  = "pass";
    s.password  = "pass";
    s.pthrottle = &throttle;

    nf::Connection::CmdListCompletionData completion;

    nf::ConnectionImpl conn;
    conn.SetCallback([&](const nf::Connection::CmdListCompletionData& data) {
        completion = data;
    });

    // resolve
    act = conn.Connect(s);
    act->set_log(log);
    act->perform();

    // Connect
    act = conn.Complete(std::move(act));
    act->set_log(log);
    act->perform();

    // initialize
    act = conn.Complete(std::move(act));
    act->set_log(log);
    act->perform();

    // successful body retrieval
    {
        nf::CmdList::Messages m;
        m.groups  = {"alt.binaries.foo"};
        m.numbers = {"3"};

        auto cmds = std::make_shared<nf::CmdList>(m);

        // execute
        act = conn.Execute(cmds, 123);
        BOOST_REQUIRE(conn.GetState() == nf::Connection::State::Active);
        act->perform();
        act->run_completion_callbacks();
        act = conn.Complete(std::move(act));
        BOOST_REQUIRE(conn.GetState() == nf::Connection::State::Connected);
        BOOST_REQUIRE(conn.GetError() == nf::Connection::Error::None);
        BOOST_REQUIRE(!act);

        BOOST_REQUIRE(completion.success);
        BOOST_REQUIRE(completion.cmds == cmds);
        BOOST_REQUIRE(completion.task_owner_id == 123);
        BOOST_REQUIRE(completion.total_bytes != 0); // todo:
        BOOST_REQUIRE(completion.content_bytes != 0); // todo:

        BOOST_REQUIRE(cmds->NumBuffers() == 1);
        BOOST_REQUIRE(cmds->GetBuffer(0).GetContentType() == nf::Buffer::Type::Article);
        BOOST_REQUIRE(cmds->GetBuffer(0).GetContentStatus()== nf::Buffer::Status::Success);
    }

    // no such body
    {
        nf::CmdList::Messages m;
        m.groups  = {"alt.binaries.foo"};
        m.numbers = {"1"};

        auto cmds = std::make_shared<nf::CmdList>(m);

        act = conn.Execute(cmds, 123);
        act->perform();
        act->run_completion_callbacks();
        act = conn.Complete(std::move(act));
        BOOST_REQUIRE(conn.GetState() == nf::Connection::State::Connected);
        BOOST_REQUIRE(conn.GetError() == nf::Connection::Error::None);

        BOOST_REQUIRE(completion.success);
        BOOST_REQUIRE(completion.cmds == cmds);
        BOOST_REQUIRE(completion.task_owner_id == 123);
        BOOST_REQUIRE(completion.total_bytes != 0); // todo:
        BOOST_REQUIRE(completion.content_bytes == 0);

        BOOST_REQUIRE(cmds->NumBuffers() == 1);
        BOOST_REQUIRE(cmds->GetBuffer(0).GetContentType() == nf::Buffer::Type::Article);
        BOOST_REQUIRE(cmds->GetBuffer(0).GetContentStatus()== nf::Buffer::Status::Unavailable);

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
