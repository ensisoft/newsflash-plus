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
#include "../message_dispatch.h"
#include "../message_register.h"

struct test_message : sdk::msgbase<test_message>
{
    std::string str;
    test_message()
    {}
};

class foo 
{
public:
    foo() : test(false)
    {
        sdk::message_dispatch::get().listen<test_message>(this);
    }
    void on_message(std::shared_ptr<test_message>& msg)
    {
        BOOST_REQUIRE(msg->str == "ardvark");
        test = true;        
    }
    bool test;
};

class bar
{
public:
    bar() : test(false)
    {
        sdk::message_dispatch::get().listen<test_message>(this);
    }
    void on_message(std::shared_ptr<test_message>& msg)
    {
        BOOST_REQUIRE(msg->str == "ardvark");
        test = true;
    }
    bool test;
};

int test_main(int, char*[])
{
    const auto id = sdk::message_register::find<test_message>();
    BOOST_REQUIRE(id);
    const auto* type = sdk::message_register::find(id);
    BOOST_REQUIRE(type);
    BOOST_REQUIRE(type->match<test_message>());

    auto msg = std::make_shared<test_message>();
    msg->str = "ardvark";

    foo f;
    bar b;

    sdk::message_dispatch::get().post(msg);

    BOOST_REQUIRE(f.test == true);
    BOOST_REQUIRE(b.test == true);

    return 0;
}