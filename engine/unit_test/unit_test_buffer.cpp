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
#include "../buffer.h"

int test_main(int, char*[])
{
    newsflash::buffer buff(1024);
    BOOST_REQUIRE(buff.size() == 0);
    BOOST_REQUIRE(buff.available() == 1024);

    const char* str = "jeesus ajaa mopolla";

    std::strcpy(buff.back(), str);
    buff.append(std::strlen(str));

    BOOST_REQUIRE(buff.size() == std::strlen(str));

    auto other = buff.split(6);

    BOOST_REQUIRE(other.size() == 6);
    BOOST_REQUIRE(buff.size() == std::strlen(str) - 6);
    BOOST_REQUIRE(!std::memcmp(buff.head(), &str[6], buff.size()));
    BOOST_REQUIRE(!std::memcmp(other.head(), "jeesus", 6));

    return 0;
}