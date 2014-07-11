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

#include <cstring>
#include <string>

#include "../message.h"
#include "../typereg.h"

struct msg_baz 
{
    std::string str;
};

struct msg_bar 
{
    bool received;
};

struct msg_foo
{
    bool received;
};

class foo 
{
public:
    foo()
    {
       sdk::listen<msg_baz>(this);
       sdk::listen<msg_foo>(this, "foo");
       recv_msg_baz = false;
    }
    void on_message(const char* sender, msg_baz& msg)
    {
        BOOST_REQUIRE(!std::strcmp(sender, "main"));
        BOOST_REQUIRE(msg.str == "aardvark");
        recv_msg_baz = true;
    }

    void on_message(const char* sender, msg_foo& msg)
    {
        BOOST_REQUIRE(!std::strcmp(sender, "main"));
        msg.received = true;
    }

    bool recv_msg_baz;
};

class bar
{
public:
    bar()
    {
        sdk::listen<msg_baz>(this);
        sdk::listen<msg_bar>(this, "bar");
        recv_msg_baz = false;        
    }
    void on_message(const char* sender, msg_baz& msg)
    {
        BOOST_REQUIRE(!std::strcmp(sender, "main"));
        BOOST_REQUIRE(msg.str == "aardvark");
        recv_msg_baz = true;
    }

    void on_message(const char* sender, msg_bar& msg)
    {
        BOOST_REQUIRE(!std::strcmp(sender, "main"));
        msg.received = true;
    }

    bool recv_msg_baz;
};

int test_main(int, char*[])
{
    using namespace sdk;

    BOOST_REQUIRE(typereg::find(0) == nullptr);
    BOOST_REQUIRE(typereg::find<msg_baz>() == 0);

    const auto id = typereg::insert<msg_baz>();
    BOOST_REQUIRE(id);
    BOOST_REQUIRE(typereg::insert<msg_baz>() == id);

    const auto* type = typereg::find(id);
    BOOST_REQUIRE(type);
    BOOST_REQUIRE(type->match<msg_baz>() == true);
    BOOST_REQUIRE(type->match<msg_bar>() == false);

    foo f;
    bar b;

    // send to everyone  
    sdk::send(msg_baz{"aardvark"}, "main");
    BOOST_REQUIRE(f.recv_msg_baz);
    BOOST_REQUIRE(b.recv_msg_baz);


    // send to foo
    msg_foo to_foo {false};
    sdk::send(&to_foo, "main", "foo");
    BOOST_REQUIRE(to_foo.received = true);

    // send to bar
    msg_bar to_bar {false};
    sdk::send(&to_bar, "main", "bar");
    BOOST_REQUIRE(to_bar.received == true);


    return 0;
}