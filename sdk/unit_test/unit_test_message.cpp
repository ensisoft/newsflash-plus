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

#include <boost/test/minimal.hpp>

#include <string>

#include "../message.h"
#include "../message_dispatcher.h"
#include "../message_register.h"

struct msg_respond : sdk::msgbase<msg_respond>
{
    std::string str;
    bool foo;
    bool bar;    
};

class foo 
{
public:
    foo()
    {
       sdk::listen<msg_respond>(this);
    }
    void on_message(msg_respond& msg)
    {
        BOOST_REQUIRE(msg.str == "ardvark");
        msg.foo = true;
    }
};

class bar
{
public:
    bar()
    {
        sdk::listen<msg_respond>(this);
    }
    void on_message(msg_respond& msg)
    {
        BOOST_REQUIRE(msg.str == "ardvark");
        msg.bar = true;
    }
};

int test_main(int, char*[])
{
    const auto id = sdk::message_register::find<msg_respond>();
    BOOST_REQUIRE(id);
    const auto* type = sdk::message_register::find(id);
    BOOST_REQUIRE(type);
    BOOST_REQUIRE(type->match<msg_respond>());

    foo f;
    bar b;

    msg_respond msg;
    msg.str = "ardvark";
    msg.foo = msg.bar = false;
    sdk::send(&msg);

    BOOST_REQUIRE(msg.foo == true);
    BOOST_REQUIRE(msg.bar == true);

    return 0;
}