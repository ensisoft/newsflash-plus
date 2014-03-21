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
#include "../buffer.h"

int test_main(int, char*[])
{
    {
        corelib::buffer buff;

        BOOST_REQUIRE(buff.capacity() == 0);
        BOOST_REQUIRE(buff.size() == 0);

        buff.reserve(100);
        BOOST_REQUIRE(buff.size() == 0);
        BOOST_REQUIRE(buff.capacity() >= 100);

        buff.resize(1);
        BOOST_REQUIRE(buff.size() == 1);
        BOOST_REQUIRE(buff.capacity() == 100);
        buff[0] = 0x10;

        buff.resize(50);
        BOOST_REQUIRE(buff.size() == 50);
        BOOST_REQUIRE(buff.capacity() == 100);
        buff[49] = 0x11;

        buff.resize(150);
        BOOST_REQUIRE(buff.size() == 150);
        BOOST_REQUIRE(buff.capacity() >= 150);
        BOOST_REQUIRE(buff[0] == 0x10);
        BOOST_REQUIRE(buff[49] == 0x11);

        buff.resize(0);
        BOOST_REQUIRE(buff.size() == 0);
        BOOST_REQUIRE(buff.empty());
    }


    {
        corelib::buffer buff;
        buff.resize(512);
        buff.offset(100);

        corelib::buffer::header header(buff);
        BOOST_REQUIRE(header.data() == buff.data());
        BOOST_REQUIRE(header.size() == 100);

        corelib::buffer::payload body(buff);
        BOOST_REQUIRE(body.data() == buff.data() + 100);
        BOOST_REQUIRE(body.size() == 412);

        body.crop(1);
        BOOST_REQUIRE(body.size() == 411);
        BOOST_REQUIRE(body.data() == buff.data() + 101);
        body.crop(2);
        BOOST_REQUIRE(body.size() == 409);
        BOOST_REQUIRE(body.data() == buff.data() + 103);
    }


    return 0;
}