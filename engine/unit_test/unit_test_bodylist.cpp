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
#include <cstring> // for memcpy
#include <vector>
#include <deque>
#include "../protocol.h"
#include "../bodylist.h"
#include "../buffer.h"

struct tester {
    std::size_t recv(void* buff, std::size_t len)
    {
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

    void body(const newsflash::bodylist::body& body)
    {
        bodies.push_back(body);
    }

    std::deque<std::string> responses;
    std::vector<newsflash::bodylist::body> bodies;
};

void unit_test_bodylist()
{
    tester test;
    test.responses = {
        "411 no such newsgroup",
        "211 12 31 33 alt.binaries.bar",
        "222 body follows",
            "bla bla bla",
            ".",
        "420 no article with that id",
        "420 dmca takedown"
    };

    newsflash::protocol proto;    
    proto.on_recv = std::bind(&tester::recv, &test,
        std::placeholders::_1, std::placeholders::_2);
    proto.on_send = std::bind(&tester::send, &test,
        std::placeholders::_1, std::placeholders::_2);

    newsflash::bodylist list = {
        {"alt.binaries.foo",
         "alt.binaries.bar"},
        {"1234", "4321", "4444"}
    };

    list.on_body = std::bind(&tester::body, &test,
        std::placeholders::_1);

    BOOST_REQUIRE(list.run(proto));
    BOOST_REQUIRE(list.run(proto) == false);

    BOOST_REQUIRE(test.bodies.size() == 3);
    const auto& body1 = test.bodies[0];
    BOOST_REQUIRE(body1.article == "1234");
    BOOST_REQUIRE(body1.buff->empty());
    BOOST_REQUIRE(body1.id == 0);
    BOOST_REQUIRE(body1.status == newsflash::bodylist::status::unavailable);

    const auto& body2 =  test.bodies[1];
    BOOST_REQUIRE(body2.article == "4321");
    BOOST_REQUIRE(body2.buff->empty() == false);    
    BOOST_REQUIRE(body2.id == 1);
    BOOST_REQUIRE(body2.status == newsflash::bodylist::status::success);

    const auto& body3 = test.bodies[2];
    BOOST_REQUIRE(body3.article == "4444");
    BOOST_REQUIRE(body3.buff->empty());
    BOOST_REQUIRE(body3.id == 2);
    BOOST_REQUIRE(body3.status == newsflash::bodylist::status::dmca);

}

int test_main(int, char*[])
{
    unit_test_bodylist();

    return 0;
}