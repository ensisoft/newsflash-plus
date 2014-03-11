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
    newsflash::buffer buff;

    BOOST_REQUIRE(buff.capacity() == 0);
    BOOST_REQUIRE(buff.size() == 0);
    BOOST_REQUIRE(buff.ptr() == nullptr);

    buff.allocate(100);
    BOOST_REQUIRE(buff.size() == 0);
    BOOST_REQUIRE(buff.capacity() == 100);
    BOOST_REQUIRE(buff.ptr());
    buff.resize(1);
    BOOST_REQUIRE(buff.size() == 1);
    buff.resize(2);
    BOOST_REQUIRE(buff.size() == 2);
    buff.resize(100);
    buff.resize(0);

    for (int i=0; i<100; ++i)
    {
        auto* ptr = buff.ptr();
        ptr[i] = 0xff;
        BOOST_REQUIRE(buff[i] == 0xff);
    }


    buff = std::move(buff);
    BOOST_REQUIRE(buff.ptr());
    BOOST_REQUIRE(buff.capacity() == 100);

    auto other = std::move(buff);

    BOOST_REQUIRE(other.ptr());
    BOOST_REQUIRE(other.capacity() == 100);
    BOOST_REQUIRE(buff.ptr() == nullptr);
    BOOST_REQUIRE(buff.capacity() == 0);

    return 0;
}