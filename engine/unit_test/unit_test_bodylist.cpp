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
#include <boost/lexical_cast.hpp>
#include <string>
#include <cstring> // for memcpy
#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <deque>
#include "../protocol.h"
#include "../bodylist.h"
#include "../buffer.h"
#include "unit_test_common.h"

struct tester {

    tester() : throw_exception(false)
    {} 

    std::size_t recv(void* buff, std::size_t len)
    {
        if (throw_exception)
            throw std::runtime_error("foobar");

        auto response = responses[0];
        response.append("\r\n");

        BOOST_REQUIRE(len >= response.size());

        std::memcpy(buff, &response[0], response.size());
        responses.pop_front();
        return response.size();
    }

    void send(const void* buff, std::size_t len)
    {}

    void log(const std::string& str)
    {}

    void body(corelib::bodylist::body&& body)
    {
        std::lock_guard<std::mutex> lock(mutex);

        bodies.push_back(std::move(body));
    }

    std::deque<std::string> responses;

    std::mutex mutex;
    std::vector<corelib::bodylist::body> bodies;
    bool throw_exception;
};

void unit_test_bodylist()
{
    // no such newsgroup
    {
        tester test;
        test.responses = 
        {
            "411 no such newsgroup",
            "411 no such newsgroup"
        };

        corelib::protocol proto;
        proto.on_recv = std::bind(&tester::recv, &test,
            std::placeholders::_1, std::placeholders::_2);
        proto.on_send = std::bind(&tester::send, &test,
            std::placeholders::_1, std::placeholders::_2);        

        corelib::bodylist list = {
            {"alt.binaries.foo", "alt.binaries.bar"},
            {"1234"}
        };

        list.on_body = std::bind(&tester::body, &test,
            std::placeholders::_1);

        list.run(proto);
        const auto& body = test.bodies.at(0);
        BOOST_REQUIRE(body.article == "1234");
        BOOST_REQUIRE(body.buff.empty());
        BOOST_REQUIRE(body.id == 0);
        BOOST_REQUIRE(body.status == corelib::bodylist::status::unavailable);        

    }

    // no such article
    {
        tester test;
        test.responses = 
        {
            "211 12 31 333 alt.binaries.bar",
            "420 no article with that id"
        };

        corelib::protocol proto;
        proto.on_recv = std::bind(&tester::recv, &test,
            std::placeholders::_1, std::placeholders::_2);
        proto.on_send = std::bind(&tester::send, &test,
            std::placeholders::_1, std::placeholders::_2);        

        corelib::bodylist list = {
            {"alt.binaries.foo"},
            {"5555"}
        };

        list.on_body = std::bind(&tester::body, &test,
            std::placeholders::_1);        

        list.run(proto);
        const auto& body = test.bodies.at(0);
        BOOST_REQUIRE(body.article == "5555");
        BOOST_REQUIRE(body.buff.offset() == 0);
        BOOST_REQUIRE(body.id == 0);
        BOOST_REQUIRE(body.status == corelib::bodylist::status::unavailable);                
    }

    // succesful article retrieval
    {
        tester test;
        test.responses = 
        {
            "211 12 31 333 alt.binaries.bar",
            "222 body follows",
                "eka body",
                ".",

            // "211 32 44 333 alt.binaries.bar", // group cmd is not sent cause it doesn't change
            "222 body follows",
                "toka body",
                "."
        };

        corelib::protocol proto;
        proto.on_recv = std::bind(&tester::recv, &test,
            std::placeholders::_1, std::placeholders::_2);
        proto.on_send = std::bind(&tester::send, &test,
            std::placeholders::_1, std::placeholders::_2);        

        corelib::bodylist list = {
            {"alt.binaries.foo"},
            {"1234", "4321"}
        };     

        list.on_body = std::bind(&tester::body, &test,
            std::placeholders::_1);        

        list.run(proto);
        list.run(proto);

        auto body = std::move(test.bodies.at(0));
        BOOST_REQUIRE(body.article == "1234");
        BOOST_REQUIRE(body.id == 0);
        BOOST_REQUIRE(body.buff.offset());
        BOOST_REQUIRE(body.status == corelib::bodylist::status::success);

        body = std::move(test.bodies.at(1));
        BOOST_REQUIRE(body.article == "4321");
        BOOST_REQUIRE(body.id == 1);
        BOOST_REQUIRE(body.buff.offset());
        BOOST_REQUIRE(body.status == corelib::bodylist::status::success);
    }

    // exception during run(), rollback semantics
    {
        tester test;
        test.responses = 
        {
            "211 1 2 3 alt.binaries.foo",
            "222 body follows",
                "foo",
                ".",

            "222 body follows",
                "bar",
                "."
        };

        corelib::protocol proto;
        proto.on_recv = std::bind(&tester::recv, &test,
            std::placeholders::_1, std::placeholders::_2);
        proto.on_send = std::bind(&tester::send, &test,
            std::placeholders::_1, std::placeholders::_2);        

        corelib::bodylist list = {
            {"alt.binaries.foo"},
            {"1234", "4321"}
        };

        list.on_body = std::bind(&tester::body, &test,
            std::placeholders::_1);

        list.run(proto);

        test.throw_exception = true;
        REQUIRE_EXCEPTION(list.run(proto));
        REQUIRE_EXCEPTION(list.run(proto));
        test.throw_exception = false;

        list.run(proto);

        auto body = std::move(test.bodies.at(0));
        BOOST_REQUIRE(body.article == "1234");

        body = std::move(test.bodies.at(1));
        BOOST_REQUIRE(body.article == "4321");
    }
}

void simulate_connection_thread(corelib::bodylist& cmdlist)
{
    struct tester {
        tester() : group_selected(false)
        {}

        size_t recv(void* buff, std::size_t len)
        {
            const char* ret = nullptr;

            if (group_selected)
                 ret = "222 body follows\r\njuuhuu\r\n.\r\n";
            else ret = "211 1 2 3 alt.biaries.bar";

            const auto retlen = std::strlen(ret);

            BOOST_REQUIRE(len >= retlen);

            std::memcpy(buff, ret, retlen);

            group_selected = true;
            return retlen;
        }

        void send(const void*, std::size_t)
        {}
        bool group_selected;
    };

    tester test;

    corelib::protocol proto;
    proto.on_recv = std::bind(&tester::recv, &test,
        std::placeholders::_1, std::placeholders::_2);
    proto.on_send = std::bind(&tester::send, &test,
        std::placeholders::_1, std::placeholders::_2);

    while (cmdlist.run(proto))
        ;
}


void unit_test_bodylist_multiple_threads()
{
    std::deque<std::string> articles;

    for (int i=0; i<1000; ++i)
        articles.push_back(boost::lexical_cast<std::string>(i));

    corelib::bodylist list = {
        {"alt.binaries.foo"},
        articles
    };
    tester test;

    list.on_body = std::bind(&tester::body, &test, std::placeholders::_1);

    std::thread t1(std::bind(simulate_connection_thread, std::ref(list)));
    std::thread t2(std::bind(simulate_connection_thread, std::ref(list)));
    std::thread t3(std::bind(simulate_connection_thread, std::ref(list)));

    t1.join();
    t2.join();
    t3.join();

    BOOST_REQUIRE(test.bodies.size() == 1000);
    // todo: check the data contents
}

int test_main(int, char*[])
{
    unit_test_bodylist();
    unit_test_bodylist_multiple_threads();

    return 0;
}