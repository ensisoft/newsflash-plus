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
#include <newsflash/warnpush.h>
#  include <boost/test/minimal.hpp>
#include <newsflash/warnpop.h>
#include <thread>
#include <deque>
#include <string>
#include <fstream>
#include <iterator>
#include "../connection.h"
#include "../action.h"
#include "../sockets.h"
#include "../socketapi.h"
#include "../cmdlist.h"
#include "../session.h"
#include "../buffer.h"
#include "../logging.h"
#include "../decode.h"
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

    nf::connection::spec s;
    s.hostname = "localhost";
    s.hostport = 1919;
    s.use_ssl  = false;
    s.enable_compression = false;
    s.enable_pipelining  = false;
    s.username  = "pass";
    s.password  = "pass";

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

    auto cmds = std::shared_ptr<nf::cmdlist>(new nf::cmdlist({"alt.binaries.foo"}, {"3"}, nf::cmdlist::type::body));

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
