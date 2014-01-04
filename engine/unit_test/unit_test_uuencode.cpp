// Copyright (c) 2013 Sami Väisänen, Ensisoft 
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

// $Id: unit_test_uuencode.cpp,v 1.11 2007/12/06 10:29:40 enska Exp $

#include <boost/test/minimal.hpp>
#include <fstream>
#include <iterator>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include "unit_test_common.h"
#include "../uuencode.h"

namespace uue = uuencode;


// Synopsis: UUEncode binary file and compare to the output of a reference encoder.
// Expected: Output matches that of the reference encoder.
void test_encode()
{
    delete_file("newsflash_2_0_0.uuencode");

    std::ifstream src;
    std::ifstream ref;
    
    src.open("test_data/newsflash_2_0_0.png", std::ios::binary);
    ref.open("test_data/newsflash_2_0_0.uuencode", std::ios::binary);

    BOOST_REQUIRE(src.is_open());
    BOOST_REQUIRE(ref.is_open());
    
    std::vector<char> buff;

    uue::begin_encoding("644", "test", std::back_inserter(buff));
    uue::encode(std::istreambuf_iterator<char>(src), std::istreambuf_iterator<char>(),
        std::back_inserter(buff));
    uue::end_encoding(std::back_inserter(buff));

    // std::ofstream out("foo.uuencode");
    // std::copy(buff.begin(), buff.end(), std::ostreambuf_iterator<char>(out));
    // out.close();

    std::vector<char> orig;
    std::copy(std::istreambuf_iterator<char>(ref), std::istreambuf_iterator<char>(), 
        std::back_inserter(orig));
    
    BOOST_REQUIRE(orig.size() == buff.size());
    BOOST_REQUIRE(std::memcmp(&orig[0], &buff[0], orig.size()) == 0);

    delete_file("newsflash_2_0_0.uuencode");    
}

// Synopsis: UUDecode text file and compared the output to the original binary.
// Expected: Output matches the original binary
void test_decode()
{
    std::ifstream src;
    std::ifstream ref;
    
    src.open("test_data/newsflash_2_0_0.uuencode", std::ios::binary);
    ref.open("test_data/newsflash_2_0_0.png", std::ios::binary);
    
    BOOST_REQUIRE(src.is_open());
    BOOST_REQUIRE(ref.is_open());
    
    std::istreambuf_iterator<char> beg(src);
    std::istreambuf_iterator<char> end;

    const auto& header = uue::parse_begin(beg, end);
    BOOST_REQUIRE(header.first);
    BOOST_REQUIRE(header.second.file == "test");
    BOOST_REQUIRE(header.second.mode == "644");
    
    std::vector<char> buff;
    BOOST_REQUIRE(uue::decode(beg, end, std::back_inserter(buff)));
    BOOST_REQUIRE(*beg == '`'); // points to the end

    uue::parse_end(beg, end);
    BOOST_REQUIRE(beg == end);
    
    std::vector<char> orig;
    std::copy(std::istreambuf_iterator<char>(ref), std::istreambuf_iterator<char>(), 
        std::back_inserter(orig));
    
    BOOST_REQUIRE(orig.size() == buff.size());
    BOOST_REQUIRE(memcmp(&orig[0], &buff[0], orig.size()) == 0);
    
}

// test various header lines
void test_parse_begin()
{
    const char* str1 = "begin 655 diu dau.png\r\n";
    const char* str2 = "begin 655 diu dau.png\n";
    const char* str3 = "begin 655 diu dau.png"; // broken sort of
    const char* str4 = "beg in foobar 655\r\n";
    const char* str5 = "";
    
    {
        const char* str = str1;
        auto ret = uue::parse_begin(str, str1+strlen(str1));

        BOOST_REQUIRE(str == str1+strlen(str1));
        BOOST_REQUIRE(ret.second.mode == "655");
        BOOST_REQUIRE(ret.second.file == "diu dau.png");
    }
    
    {
        const char* str = str2;
        auto ret = uue::parse_begin(str, str2+strlen(str2));
        
        BOOST_REQUIRE(str == str2+strlen(str2));
        BOOST_REQUIRE(ret.second.mode == "655");
        BOOST_REQUIRE(ret.second.file == "diu dau.png");
    }

    {
        const char* str = str3;
        auto ret = uue::parse_begin(str, str3+strlen(str3));
        
        BOOST_REQUIRE(str == str3+strlen(str3));
        BOOST_REQUIRE(ret.second.mode == "655");
        BOOST_REQUIRE(ret.second.file == "diu dau.png");
    }
    
    {
        const char* str = str4;
        auto ret = uue::parse_begin(str, str4+strlen(str4));

        //BOOST_REQUIRE(str == str4);
        BOOST_REQUIRE(ret.first == false);
    }
    
    {
        const char* str = str5;
        auto ret = uue::parse_begin(str, str5+strlen(str5));
        
        BOOST_REQUIRE(ret.first == false);

    }
}


// test that decoding stops at the right spot
void test_stop_decode()
{
    {
        const char* str = "M<(-W0Y/'%<B^H'DYZ=*K37Y8*!CD`]??_P\"M5JBVQ<Z6AU4VMECA%5!Z@U0F\r\n"\
                          "MU69V),I`'O\\`XBN>>^)R,GT`-02739X/!K58>Q'M&;CWVYB2V3ZGZ56DU#`)\r\n"\
                          "M)XZBL8S-RS$\\_*,GMCVIN\\G(Y&./3K5JDA<[-)KXL<`[>W3.?<?RJN]T[`D\r\n"\
                          "M'MP!Q5+>>,[AD\\#]*4-EN0/F.<#^E:<J1',2B0@89B2.XY__`%TI<L$`)8]2\r\n\r\n"\
                          "----== Posted via Newsfeeds.Com - Unlimited-Unrestricted-Secure Usenet News==----\r\n"\
                          "http://www.newsfeeds.com The #1 Newsgroup Service in the World! 120,000+ Newsgroups\r\n"\
                          "----= East and West-Coast Server Farms - Total Privacy via Encryption =----\r\n";
    
        const char* beg = str;
        const char* end = str + std::strlen(str);

        std::vector<char> buff;
        uuencode::decode(beg, end, std::back_inserter(buff));
    
        BOOST_REQUIRE(!std::strncmp(beg, "\r\n----== Posted via Newsfeeds.com", 20));
    }

    {
        const char* str = "M.\"3R<9_K3P<$\\#&.@J9\"6&,G!YYY]*5DVR!2=P![_A6/KH`NH\\#`\\L<?B:__\r\n"
                          "!V0``\r\n"
                          "`\r\nend";

        const char* beg = str;
        const char* end = str + std::strlen(str);

        std::vector<char> buff;
        uuencode::decode(beg, end, std::back_inserter(buff));

        BOOST_REQUIRE(!std::strncmp(beg, "`\r\nend", 6));
    }
}

int test_main(int, char* [])
{
    test_encode();
    test_decode();
    test_parse_begin();
    test_stop_decode();


    return 0;
}
