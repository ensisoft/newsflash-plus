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
#include "../bodyiter.h"

void test_bodyiter()
{
    // if a textual body response constains a double as the first character on a 
    // new line the dot is doubled. (".\r\n" indicates the end of transmission)

    // test forward
    {
        const char* text = 
        "..here is some text\r\n"
        "more data follows.\r\n"
        "two dots in the middle .. of text\r\n"
        "and no dots\r\n"
        "..\r\n"
        "...\r\n"
        "....\r\n";

        nntp::bodyiter beg(text);
        nntp::bodyiter end(text + std::strlen(text));

        std::string str(beg, end);
        BOOST_REQUIRE( str == 
            ".here is some text\r\n"
            "more data follows.\r\n"
            "two dots in the middle .. of text\r\n"
            "and no dots\r\n"
            ".\r\n"
            "..\r\n"
            "...\r\n"
            );

    }

    // test backwards
    {
        const char* text = 
        "..here is some text\r\n"
        "more data follows.\r\n"
        "two dots in the middle .. of text\r\n"
        "and no dots\r\n"
        "..\r\n"
        "...\r\n"
        "....\r\n";

        nntp::bodyiter beg(text - 1);
        nntp::bodyiter end(text, std::strlen(text)-1);

        std::string str;
        for (; end != beg; --end)
        {
            str.insert(str.begin(), *end);
        }

        BOOST_REQUIRE( str == 
            ".here is some text\r\n"
            "more data follows.\r\n"
            "two dots in the middle .. of text\r\n"
            "and no dots\r\n"
            ".\r\n"
            "..\r\n"
            "...\r\n"
            );
    }

    // test forward and backwards
    {
        const char* text =
        "..here is some text\r\n"
        "..\r\n"
        "...\r\n";

        nntp::bodyiter beg(text);

        std::string str;
        str.push_back(*beg++);
        str.push_back(*beg++);
        str.push_back(*--beg);
        str.push_back(*--beg);
        BOOST_REQUIRE(str == ".hh.");

        str.clear();

        std::advance(beg, std::strlen(".here is some text\r\n"));
        str.push_back(*beg++);
        str.push_back(*beg++);
        str.push_back(*beg++);
        BOOST_REQUIRE(str == ".\r\n");

        str.clear();
        str.push_back(*--beg);
        str.push_back(*--beg);
        str.push_back(*--beg);
        BOOST_REQUIRE(str == "\n\r.");
    }    
}

int test_main(int, char*[])
{
    test_bodyiter();

    return 0;
}