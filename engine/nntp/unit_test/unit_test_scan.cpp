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
#include "../scan.h"

void test_scan_response()
{
    {
        int a, b, c;
        BOOST_REQUIRE(nntp::scan_response("123 234254 444", a, b, c));
        BOOST_REQUIRE(a == 123);
        BOOST_REQUIRE(b == 234254);
        BOOST_REQUIRE(c == 444);
    }

    {
        int a; 
        double d;
        BOOST_REQUIRE(nntp::scan_response("200 40.50", a, d));
        BOOST_REQUIRE(a == 200);
        //BOOST_REQUIRE(d == 40.50); // fix
    }

    {
        int a;
        std::string s;
        BOOST_REQUIRE(nntp::scan_response("233 welcome posting allowed", a, s));
        BOOST_REQUIRE(a == 233);
        BOOST_REQUIRE(s == "welcome");
    }

    {
        int a;
        nntp::trailing_comment c;
        BOOST_REQUIRE(nntp::scan_response("233 welcome posting allowed", a, c));
        BOOST_REQUIRE(a == 233);
        BOOST_REQUIRE(c.str == "welcome posting allowed");
    }

    {
        int a;
        BOOST_REQUIRE(!nntp::scan_response("asdga", a));
    }
}


int test_main(int, char*[])
{
    test_scan_response();
    return 0;
}