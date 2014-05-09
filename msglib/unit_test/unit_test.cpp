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
#include <thread>
#include "../message.h"
#include "../queue.h"

struct foo {
    std::string str;
    int i;
};

struct bar {};


void unit_test_message()
{
    msglib::message msg(1, foo {"kekek", 1234});

    BOOST_REQUIRE(msg.id() == 1);
    BOOST_REQUIRE(msg.test<bar>() == false);
    BOOST_REQUIRE(msg.test<foo>() == true);

    BOOST_REQUIRE(msg.as<foo>().str == "kekek");
    BOOST_REQUIRE(msg.as<foo>().i == 1234);
}

void unit_test_queue()
{
    typedef msglib::queue<foo> foo_queue;

    {
        foo_queue f;
        foo_queue::message m;
        BOOST_REQUIRE(f.get(m) == false);
    }

    {
        foo msg = {"keke", 1234};

        foo_queue f;
        foo_queue::message m;

        f.post(msg);
        BOOST_REQUIRE(f.get(m) == true);
        BOOST_REQUIRE(m.value.i == 1234);
        BOOST_REQUIRE(m.value.str == "keke");
    }

    {
        foo_queue f;
        foo_queue::message m;

        f.emplace_post("jallukola", 1234);
        BOOST_REQUIRE(f.get(m) == true);
        BOOST_REQUIRE(m.value.i == 1234);
        BOOST_REQUIRE(m.value.str == "jallukola");
    }

    {
        foo msg = {"jeesus ajaa mopolla", 4444};

        foo_queue f;
        foo_queue::message m;

        auto later = f.send(msg);
        BOOST_REQUIRE(later.ready() == false);

        f.get(m);
        m.dispose();
        BOOST_REQUIRE(later.ready() == true);
    }

    {
        typedef msglib::queue<msglib::message> msg_queue;

        msg_queue q;
        msg_queue::message m;

        q.emplace_post(123, foo {"ardvark", 5555});
        q.get(m);
        BOOST_REQUIRE(m.value.id() == 123);
    }
}

void unit_test_queue_post()
{
    typedef msglib::queue<foo> foo_queue;

    //foo_queue queue;

    std::thread producer(
        [&]() {
            for (int i=0; i<10000; ++i)
            {
                //queue.emplace_post("foo", i);
            }
        });

    std::thread consumer(
        [&]() {
            for (int i=0; i<10000; ++i)
            {
                // foo_queue::message m;
                // while (!queue.get(m));

                // BOOST_REQUIRE(m.value.i == i);
                // BOOST_REQUIRE(m.value.str == "foo");
            }
        });

    producer.join();
    consumer.join();
}

int test_main(int, char*[])
{
    unit_test_message();
    unit_test_queue();
    unit_test_queue_post();

    return 0;
}