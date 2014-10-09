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
#include "../linebuffer.h"

void test_linebuffer()
{
    // no lines
    {
        const char* str = 
           "fooobooar";

        nntp::linebuffer buffer(str, std::strlen(str));
        auto beg = buffer.begin();
        auto end = buffer.end();
        BOOST_REQUIRE(beg == end);
    }

    // some lines
    {
        const char* str = 
           "assa\r\n"
           "sassa\r\n"
           "mandelmassa\r\n"
           "foo\r\n";

        nntp::linebuffer buffer(str, std::strlen(str));

        auto beg = buffer.begin();
        auto end = buffer.end();
        BOOST_REQUIRE(beg.to_str() == "assa\r\n");
        ++beg;
        BOOST_REQUIRE(beg.to_str() == "sassa\r\n");
        beg++;
        BOOST_REQUIRE(beg.to_str() == "mandelmassa\r\n");
        ++beg;
        BOOST_REQUIRE(beg.to_str() == "foo\r\n");
        beg++;
        BOOST_REQUIRE(beg == end);
    }

    // last line is not full
    {
        const char* str = 
           "jeesus\r\n"
           "ajaa\r\n"
           "mopolla";

        nntp::linebuffer buffer(str, std::strlen(str));

        auto beg = buffer.begin();
        auto end = buffer.end();
        BOOST_REQUIRE(beg.to_str() == "jeesus\r\n");
        ++beg;
        BOOST_REQUIRE(beg.to_str() == "ajaa\r\n");
        ++beg;
        BOOST_REQUIRE(beg == end);
    }

    // only a single "empty" line
    {
        const char* str = "\r\n";

        nntp::linebuffer buffer(str, std::strlen(str));

        auto beg = buffer.begin();
        auto end = buffer.end();
        BOOST_REQUIRE(beg.to_str() == "\r\n");
        ++beg;
        BOOST_REQUIRE(beg == end);
    }

    // mixed "\r\n" and just "\n"
    {
        const char* str = "foo\r\nbar\nkeke\n";

        nntp::linebuffer buffer(str, std::strlen(str));

        auto beg = buffer.begin();
        auto end = buffer.end();
        BOOST_REQUIRE(beg.to_str() == "foo\r\n");
        ++beg;
        BOOST_REQUIRE(beg.to_str() == "bar\n");
        ++beg;
        BOOST_REQUIRE(beg.to_str() == "keke\n");
    }
}

int test_main(int, char*[])
{
    test_linebuffer();
    return 0;
}