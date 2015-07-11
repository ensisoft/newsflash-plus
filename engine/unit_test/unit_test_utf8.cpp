// Copyright (c) 2010-2013 Sami Väisänen, Ensisoft 
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
#include <newsflash/warnpush.h>
#  include <boost/test/minimal.hpp>
#include <newsflash/warnpop.h>
#include <cstdint>
#include <vector>
#include <iterator>
#include <string>
#include "../utf8.h"

using namespace std;

    typedef uint32_t ucs4;

void test_success()
{
    // english (latin1)
    {
        // quick brown fox
        const ucs4 input[] = {0x71, 0x75, 0x69, 0x63, 0x6b, 0x20, 0x62, 0x72, 0x6f, 0x77, 0x6e, 0x20, 0x66, 0x6f, 0x78};
        // utf-8
        const unsigned char utf8[] = {'q', 'u', 'i', 'c', 'k', ' ', 'b', 'r', 'o', 'w', 'n', ' ', 'f', 'o', 'x', 0};
        
        basic_string<unsigned char> ret;
        utf8::encode(input, input+sizeof(input)/sizeof(ucs4), back_inserter(ret));
        BOOST_REQUIRE(ret == utf8);
        
        vector<ucs4> v;
        utf8::decode<ucs4>(ret.begin(), ret.end(), back_inserter(v));
        BOOST_REQUIRE(v.size() != 0);
        BOOST_REQUIRE(memcmp(&v[0], input, v.size())==0);
    }

    // european text with diatrics
    {
        // ääliö ääpinen
        const ucs4 input[] = {0xe4, 0xe4, 0x6c, 0x69, 0xf6, 0x20, 0xe4, 0xe4, 0x70, 0x69, 0x6e, 0x65, 0x6e};
        // utf-8
        const unsigned char utf8[] = {0xc3, 0xa4, 0xc3, 0xa4, 'l', 'i', 0xc3, 0xb6, ' ', 0xc3, 0xa4, 0xc3, 0xa4, 'p', 'i', 'n', 'e', 'n', 0};
        
        basic_string<unsigned char> ret;
        utf8::encode(input, input+sizeof(input)/sizeof(ucs4), back_inserter(ret));
        BOOST_REQUIRE(ret == utf8);
              
        vector<ucs4> v;
        utf8::decode<ucs4>(ret.begin(), ret.end(), back_inserter(v));
        BOOST_REQUIRE(v.size() != 0);
        BOOST_REQUIRE(memcmp(&v[0], input, v.size())==0);
    }

    // some japanese
    {
        // わたしわさみ
        const ucs4 input[] = {0x308f, 0x305f, 0x3057, 0x308f, 0x3055, 0x307f};
        // utf-8
        const unsigned char utf8[] = {0xe3, 0x82, 0x8f, 0xe3, 0x81, 0x9f, 0xe3, 0x81, 0x97, 0xe3, 0x82, 0x8f, 0xe3, 0x81, 0x95, 0xe3, 0x81, 0xbf, 0};
        
        basic_string<unsigned char> ret;
        utf8::encode(input, input+sizeof(input)/sizeof(ucs4), back_inserter(ret));
        BOOST_REQUIRE(ret == utf8);
        
        vector<ucs4> v;
        utf8::decode<ucs4>(ret.begin(), ret.end(), back_inserter(v));
        BOOST_REQUIRE(v.size() != 0);
        BOOST_REQUIRE(memcmp(&v[0], input, v.size())==0);

    }
    // unmapped code points
    {
        const ucs4 input[] = {0x10001, 0x10002, 0x10003};
        const unsigned char utf8[]  = {0xf0, 0x90, 0x80, 0x81, 0xf0, 0x90, 0x80, 0x82, 0xf0, 0x90, 0x80, 0x83, 0};
        
        std::basic_string<unsigned char> ret;
        utf8::encode(input, input+sizeof(input)/sizeof(ucs4), back_inserter(ret));
        BOOST_REQUIRE(ret == utf8);
        
        vector<ucs4> v;
        utf8::decode<ucs4>(ret.begin(), ret.end(), back_inserter(v));
        BOOST_REQUIRE(v.size() != 0);
        BOOST_REQUIRE(memcmp(&v[0], input, v.size())==0);

    }

    {
        // signed char, very common possibility
        const char input[] = {(char)0xe4}; // ä
        
        const unsigned char utf8[] = {0xc3, 0xa4, 0};
        
        std::basic_string<unsigned char> ret;
        utf8::encode(input, input + sizeof(input)/sizeof(char), back_inserter(ret));
        BOOST_REQUIRE(ret == utf8);
    }
}


void test_failure()
{
    // invalid bytes
    {
        // this is a 5 byte sequence that would decode to a value higher than 0x10ffff
        const unsigned char utf8[] = {
            0xf8, 0x80, 0x80, 0x80, 0x80
        };
        bool success = true;
        utf8::decode((const char*)utf8, &success);
        BOOST_REQUIRE(success == false);
    }   

    // invalid continuation byte without a leading byte
    {
        // € sign + 0x90
        const unsigned char utf8[] = {
            0xe2, 0x82, 0xac, 0x90
        };

        bool success = true;
        utf8::decode((const char*)utf8, &success);
        BOOST_REQUIRE(success == false);
    }

    // a start byte without enough continuation bytes.
    {
        // € sign
        const unsigned char utf8[] = {
            0xe2, 0x82 //, 0xac
        };
        bool success = true;
        utf8::decode((const char*)utf8, &success);
        BOOST_REQUIRE(success == false);
    }
}

int test_main(int, char* [])
{
    test_success();
    test_failure();

    return 0;
}
