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
#include <thread>
#include "../xoverlist.h"
#include "../protocol.h"
#include "unit_test_common.h"

struct tester {

    tester() : xover_count(-1), throw_exception(false), group_unavailable(false)
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

    void xover(newsflash::xoverlist::xover&& xover)
    {
        std::lock_guard<std::mutex> lock(mutex);

        xovers.push_back(std::move(xover));
    }

    void unavailable()
    {
        std::lock_guard<std::mutex> lock(mutex);
        group_unavailable = true;        
    }

    void prepare(std::size_t count)
    {
        std::lock_guard<std::mutex> lock(mutex);
        xover_count = count;        
    }

    std::deque<std::string> responses;

    std::mutex mutex;
    std::vector<newsflash::xoverlist::xover> xovers;
    std::size_t xover_count;
    bool throw_exception;
    bool group_unavailable;
};

void test_xoverlist()
{
    // no such newsgroup
    {
        tester test;
        test.responses = 
        {
            "411 no such newsgroup"
        };

        newsflash::protocol proto;
        proto.on_recv = std::bind(&tester::recv, &test, 
            std::placeholders::_1, std::placeholders::_2);
        proto.on_send = std::bind(&tester::send, &test,
            std::placeholders::_1, std::placeholders::_2);

        newsflash::xoverlist list = {"alt.binaries.foo"};
        list.on_unavailable = std::bind(&tester::unavailable, &test);

        BOOST_REQUIRE(list.run(proto) == false);
        BOOST_REQUIRE(test.group_unavailable);
    }

    // empty group
    {
        tester test;
        test.responses = 
        {
            "211 0 0 0  alt.binaries.foo"
        };

        newsflash::protocol proto;
        proto.on_recv = std::bind(&tester::recv, &test, 
            std::placeholders::_1, std::placeholders::_2);
        proto.on_send = std::bind(&tester::send, &test,
            std::placeholders::_1, std::placeholders::_2);

        newsflash::xoverlist list = {"alt.binaries.foo"};
        list.on_prepare_ranges = std::bind(&tester::prepare, &test, std::placeholders::_1);

        BOOST_REQUIRE(list.run(proto) == false);
        BOOST_REQUIRE(test.xover_count == 0);
    }

    // group with 1 article (with article number 2)
    {
        tester test;
        test.responses = 
        {
            "211 1 2 2 alt.binaries.foo",
            "224 overview information follows",
                 "overview data",
                 "."
        };

        newsflash::protocol proto;
        proto.on_recv = std::bind(&tester::recv, &test, 
            std::placeholders::_1, std::placeholders::_2);
        proto.on_send = std::bind(&tester::send, &test,
            std::placeholders::_1, std::placeholders::_2);

        newsflash::xoverlist list = {"alt.binaries.foo"};
        list.on_prepare_ranges = std::bind(&tester::prepare, &test, std::placeholders::_1);
        list.on_xover = std::bind(&tester::xover, &test, std::placeholders::_1);

        list.run(proto);

        BOOST_REQUIRE(test.xover_count == 1);
        BOOST_REQUIRE(test.xovers.size() == 1);
        auto xover = std::move(test.xovers.at(0));
        BOOST_REQUIRE(xover.start == 2);
        BOOST_REQUIRE(xover.end == 2);        
    }

    // group with multiple articles, within a single range
    {
        tester test;
        test.responses = 
        {
            "211 405 100 504 alt.binaries.foo",
            "224 overview information follows",
                "overview data",
                "."
        };

        newsflash::protocol proto;
        proto.on_recv = std::bind(&tester::recv, &test, 
            std::placeholders::_1, std::placeholders::_2);
        proto.on_send = std::bind(&tester::send, &test,
            std::placeholders::_1, std::placeholders::_2);

        newsflash::xoverlist list = {"alt.binaries.foo"};
        list.on_prepare_ranges = std::bind(&tester::prepare, &test, std::placeholders::_1);
        list.on_xover = std::bind(&tester::xover, &test, std::placeholders::_1);

        list.run(proto);

        BOOST_REQUIRE(test.xover_count == 1);
        BOOST_REQUIRE(test.xovers.size() == 1);
        auto xover = std::move(test.xovers.at(0));
        BOOST_REQUIRE(xover.start == 100);
        BOOST_REQUIRE(xover.end == 504);          
    }

    // group with multiple articles, multiple ranges
    {
        tester test;
        test.responses = 
        {
            "211 3016 100 3115 alt.binaries.foo",
            "224 overview information follows",
                "overview data",
                ".",
            "224 overview information follows",
                "bla bla bla",
                "."
        };

        newsflash::protocol proto;
        proto.on_recv = std::bind(&tester::recv, &test, 
            std::placeholders::_1, std::placeholders::_2);
        proto.on_send = std::bind(&tester::send, &test,
            std::placeholders::_1, std::placeholders::_2);

        newsflash::xoverlist list = {"alt.binaries.foo"};
        list.on_prepare_ranges = std::bind(&tester::prepare, &test, std::placeholders::_1);
        list.on_xover = std::bind(&tester::xover, &test, std::placeholders::_1);

        list.run(proto);
        list.run(proto);

        BOOST_REQUIRE(test.xover_count == 2);
        BOOST_REQUIRE(test.xovers.size() == 2);
        auto xover = std::move(test.xovers.at(0));
        BOOST_REQUIRE(xover.start == 100);
        BOOST_REQUIRE(xover.end == 2099);          
        xover = std::move(test.xovers.at(1));
        BOOST_REQUIRE(xover.start == 2100);
        BOOST_REQUIRE(xover.end == 3115);
    }

    // exception during operation, check roollback semantics
    {
        tester test;
        test.responses = 
        {
            "211 3016 100 3115 alt.binaries.foo",
            "224 overview information follows",
                "overview data",
                ".",
            "224 overview information follows",
                "bla bla bla",
                "."
        };

        newsflash::protocol proto;
        proto.on_recv = std::bind(&tester::recv, &test, 
            std::placeholders::_1, std::placeholders::_2);
        proto.on_send = std::bind(&tester::send, &test,
            std::placeholders::_1, std::placeholders::_2);

        newsflash::xoverlist list = {"alt.binaries.foo"};
        list.on_prepare_ranges = std::bind(&tester::prepare, &test, std::placeholders::_1);
        list.on_xover = std::bind(&tester::xover, &test, std::placeholders::_1);

        list.run(proto);
        test.throw_exception = true;
        REQUIRE_EXCEPTION(list.run(proto));
        REQUIRE_EXCEPTION(list.run(proto));
        test.throw_exception = false;
        list.run(proto);

        BOOST_REQUIRE(test.xover_count == 2);
        BOOST_REQUIRE(test.xovers.size() == 2);
        auto xover = std::move(test.xovers.at(0));
        BOOST_REQUIRE(xover.start == 100);
        BOOST_REQUIRE(xover.end == 2099);          
        xover = std::move(test.xovers.at(1));
        BOOST_REQUIRE(xover.start == 2100);
        BOOST_REQUIRE(xover.end == 3115);        
    }
}

std::mutex mutex;
std::condition_variable cond;
bool done;

void simulate_connection_thread(newsflash::xoverlist& xover)
{
    struct tester {
        tester() : group_selected(false)
        {}

        size_t recv(void* buff, size_t len)
        {
            const char* ret = nullptr;
            if (group_selected)
                 ret = "224 overview information follows\r\nblablabla\r\n.\r\n";
            else ret = "211 45000 1 45000 alt.binaries.foo\r\n";

            const auto retlen = std::strlen(ret);
            BOOST_REQUIRE(len >= retlen);
            std::memcpy(buff, ret, retlen);

            group_selected = true;
            return retlen;
        }
        void send(const void*, size_t len)
        {}

        bool group_selected;
    };

    {
        std::unique_lock<std::mutex> lock(mutex);
        while (!done)
            cond.wait(lock);
    }

    tester test;

    newsflash::protocol proto;
    proto.on_recv = std::bind(&tester::recv, &test,
        std::placeholders::_1, std::placeholders::_2);
    proto.on_send = std::bind(&tester::send, &test,
        std::placeholders::_1, std::placeholders::_2);

    while (xover.run(proto))
        ;    
}

void test_xoverlist_multiple_threads()
{
    std::unique_lock<std::mutex> lock(mutex);

    newsflash::xoverlist list = {"alt.binaries.foo"};

    tester test;

    list.on_xover = std::bind(&tester::xover, &test, std::placeholders::_1);

    std::thread t1(std::bind(simulate_connection_thread, std::ref(list)));
    std::thread t2(std::bind(simulate_connection_thread, std::ref(list)));
    std::thread t3(std::bind(simulate_connection_thread, std::ref(list)));

    done = true;
    lock.unlock();
    cond.notify_all();

    t1.join();
    t2.join();
    t3.join();

    BOOST_REQUIRE(test.xovers.size() == (45000 + (2000 - 1)) / 2000);

}

int test_main(int, char*[])
{
    test_xoverlist();
    test_xoverlist_multiple_threads();

    return 0;
}