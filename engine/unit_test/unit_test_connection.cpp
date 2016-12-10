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
        BOOST_REQUIRE(act->has_exception());
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
        BOOST_REQUIRE(!act->has_exception());

        // connect
        act = conn.complete(std::move(act));
        act->set_log(log);
        act->perform();
        BOOST_REQUIRE(act->has_exception());
    }

    // nntp init fails
    // {

    //     nf::connection::spec s;
    //     s.hostname = "www.google.com";
    //     s.hostport = 80;
    //     s.use_ssl  = false;

    //     nf::connection conn;

    //     // resolve
    //     act = conn.connect(s);
    //     act->set_log(log);
    //     act->perform(); // resolve
    //     act = conn.complete(std::move(act));

    //     // connect
    //     act->set_log(log);
    //     act->perform(); // connect
    //     act = conn.complete(std::move(act));

    //     // initialize
    //     act->set_log(log);
    //     act->perform();
    //     BOOST_REQUIRE(act->has_exception());
    // }

    // authentication fails
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
        BOOST_REQUIRE(act->has_exception());
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
        BOOST_REQUIRE(!act->has_exception());

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


        act = conn.disconnect();
        act->set_log(log);
        act->perform();
        BOOST_REQUIRE(!act->has_exception());
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

void test_execute()
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


    nf::connection conn;

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

    nf::cmdlist::messages m;
    m.groups  = {"alt.binaries.foo"};
    m.numbers = {"3"};

    auto cmds = std::make_shared<nf::cmdlist>(m);

    // execute
    act = conn.execute(cmds, 123);
    act->perform();
    act = conn.complete(std::move(act));
    BOOST_REQUIRE(!act);
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
