// Copyright (c) 2013,2014 Sami Väisänen, Ensisoft 
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


// $Id: unit_test_yenc.cpp,v 1.12 2008/06/10 18:40:56 enska Exp $

#include <boost/test/minimal.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include "../yenc.h"

// test parsing headers, parts and end lines
void test_parsing()
{
    {
        const char* str = "=ybegin part=1 line=128 size=824992 name=Black Sabbath 1970 - 1998.vol000+01.PAR2";

        const auto& ret = yenc::parse_header(str, str+strlen(str));

        BOOST_REQUIRE(ret.first == true);
        BOOST_REQUIRE(ret.second.part == 1);
        BOOST_REQUIRE(ret.second.line == 128);
        BOOST_REQUIRE(ret.second.size == 824992);
        BOOST_REQUIRE(ret.second.name == "Black Sabbath 1970 - 1998.vol000+01.PAR2");
    }

    {
        std::string str("=ybegin part=1 line=128 size=824992 name=Black Sabbath 1970 - 1998.vol000+01.PAR2");

        auto beg = str.begin();

        const auto& ret = yenc::parse_header(beg, str.end());

        BOOST_REQUIRE(ret.first == true);
        BOOST_REQUIRE(ret.second.part == 1);
        BOOST_REQUIRE(ret.second.line == 128);
        BOOST_REQUIRE(ret.second.size == 824992);
        BOOST_REQUIRE(ret.second.name == "Black Sabbath 1970 - 1998.vol000+01.PAR2");

    }

    {

        const char* str = "=ybegin line=128 size=824992 name=Black Sabbath 1970 - 1998.vol000+01.PAR2";

        const auto& ret = yenc::parse_header(str, str+strlen(str));

        BOOST_REQUIRE(ret.first == true);
        BOOST_REQUIRE(ret.second.part == 0);
        BOOST_REQUIRE(ret.second.line == 128);
        BOOST_REQUIRE(ret.second.size == 824992);

    }

    {
        const char* str = "=yend size=16154 crc32=4a68571d";
        
        const auto& ret = yenc::parse_end(str, str+strlen(str));

        BOOST_REQUIRE(ret.first == true);
        BOOST_REQUIRE(ret.second.size == 16154);
        BOOST_REQUIRE(ret.second.crc32 ==1248352029);
        BOOST_REQUIRE(ret.second.part == 0);
        BOOST_REQUIRE(ret.second.pcrc32 == 0);
        
    }

    {
        // single part yend
        const char* str = "=yend size=16154";
        
        const auto& ret = yenc::parse_end(str, str+strlen(str));

        BOOST_REQUIRE(ret.first == true);
        BOOST_REQUIRE(ret.second.size == 16154);
        BOOST_REQUIRE(ret.second.crc32== 0);
        BOOST_REQUIRE(ret.second.part == 0);
        BOOST_REQUIRE(ret.second.pcrc32 == 0);
        
    }

    {
        // multipart end
        const char* str = "=yend size=16154 pcrc32=4a68571d";

        const auto& ret = yenc::parse_end(str, str+strlen(str));

        BOOST_REQUIRE(ret.first == true);
        BOOST_REQUIRE(ret.second.size == 16154);
        BOOST_REQUIRE(ret.second.pcrc32==1248352029);
        BOOST_REQUIRE(ret.second.part == 0);
        BOOST_REQUIRE(ret.second.crc32 == 0);
        
    }


    {
        const char* str = "=ybegin part=1 line=128 size=824992 name=foobar.PAR2\r\n" \
                          "=ypart begin=12345 end=54321\r\n";
        
        const auto& header = yenc::parse_header(str, str+strlen(str));
        BOOST_REQUIRE(header.first == true);
        BOOST_REQUIRE(header.second.part == 1);
        BOOST_REQUIRE(header.second.line == 128);
        BOOST_REQUIRE(header.second.size == 824992);
        BOOST_REQUIRE(header.second.name == "foobar.PAR2");
        BOOST_REQUIRE(header.second.total == 0);

        const auto& part = yenc::parse_part(str, str+strlen(str));
        BOOST_REQUIRE(part.first == true);
        BOOST_REQUIRE(part.second.begin == 12345);
        BOOST_REQUIRE(part.second.end   == 54321);

    }

    {
        const char* str = "=ypart begin=123 end=100\r\n\vfoo";

        const auto& part = yenc::parse_part(str, str+strlen(str));

        BOOST_REQUIRE(part.first == true);
        BOOST_REQUIRE(*str == '\v');

    }

    {
        const char* str = "=ypart begin=123 end=100\r\n foo";
  
        const auto& part = yenc::parse_part(str, str+strlen(str));

        BOOST_REQUIRE(part.first == true);
        BOOST_REQUIRE(*str == ' ');

    }

    {

        const char* str = "=ybegin part=1 line=128 size=1497442 name=Nokia Smartphone Hacks - "\
                          "eBook - Must have für alle Handy Fans - 2007 German.part08.rar\r\n"\
                          "=ypart begin=1 end=384000"; 

        const auto& header = yenc::parse_header(str, str+strlen(str));

        BOOST_REQUIRE(header.first == true);
        BOOST_REQUIRE(header.second.part == 1);
        BOOST_REQUIRE(header.second.line == 128);
        BOOST_REQUIRE(header.second.size == 1497442);
        BOOST_REQUIRE(header.second.total == 0);
        BOOST_REQUIRE(header.second.name == "Nokia Smartphone Hacks - eBook - Must have für alle Handy Fans - 2007 German.part08.rar");
    
        const auto& part = yenc::parse_part(str, str+strlen(str));
        BOOST_REQUIRE(part.first == true);
        BOOST_REQUIRE(part.second.begin == 1);
        BOOST_REQUIRE(part.second.end == 384000);
        
    }

    {
        // order of the parameters is changed

        const char* str = "=ybegin part=1  total=4 size=1507728 line=128 name=(null)";
        
        const auto& ret = yenc::parse_header(str, str+strlen(str));
        BOOST_REQUIRE(ret.first == true);
        BOOST_REQUIRE(ret.second.part  == 1);
        BOOST_REQUIRE(ret.second.total == 4);
        BOOST_REQUIRE(ret.second.size  == 1507728);
        BOOST_REQUIRE(ret.second.line  == 128);
        BOOST_REQUIRE(ret.second.name  == "(null)");

    }

    {
        const char* str = "=ybegin asglasjasa part=123\r\n";

        const auto& header = yenc::parse_header(str, str+strlen(str));
        BOOST_REQUIRE(header.first == false);
    }

    {
        const char* str = "=ypart begin=sljsdg";

        const auto& part = yenc::parse_part(str, str+strlen(str));
        BOOST_REQUIRE(part.first == false);
    }

    {
        const char* str = "=yend size=sss123sss\r\n";

        const auto& end = yenc::parse_end(str, str+strlen(str));
        BOOST_REQUIRE(end.first == false);
    }
}

/*
 * Synopsis: yEnc decode the reference text file and compare to the original binary.
 * 
 * Expected: Decoded output and the binary data match.
 *
 */
void test_decoding()
{
    std::ifstream src;
    std::ifstream ref;
    
    src.open("test_data/newsflash_2_0_0.yenc", std::ios::binary);
    ref.open("test_data/newsflash_2_0_0.png", std::ios::binary);
    
    BOOST_REQUIRE(src.is_open());
    BOOST_REQUIRE(ref.is_open());
    
    std::vector<char> temp;
    std::vector<char> orig;
    std::copy(std::istreambuf_iterator<char>(src), std::istreambuf_iterator<char>(), std::back_inserter(temp));
    std::copy(std::istreambuf_iterator<char>(ref), std::istreambuf_iterator<char>(), std::back_inserter(orig));

    auto it = temp.begin();

    const auto& header = yenc::parse_header(it, temp.end());
    BOOST_REQUIRE(header.first);
    BOOST_REQUIRE(header.second.part == 0);
    BOOST_REQUIRE(header.second.line == 128);
    BOOST_REQUIRE(header.second.name == "test.png");

    std::vector<char> data;
    yenc::decode(it, temp.end(), std::back_inserter(data));

    BOOST_REQUIRE(orig.size() == data.size());
    BOOST_REQUIRE(std::memcmp(&orig[0], &data[0], orig.size()) == 0);

    // vector<char> data2;
    // yenc_decode_data(beg, temp.end(), data2);
    // BOOST_REQUIRE(data.size() == data2.size());
    // BOOST_REQUIRE(data == data2);
        
}

/*
 * Synopsis: yEnc encode the reference binary into yEnc text stream and compare to the reference yEnc data.
 * 
 * Expected: Encoded output and the reference yEnc match.
 */
void test_encoding()
{
    std::ifstream ref;
    std::ifstream src;
    ref.open("test_data/newsflash_2_0_0.yenc", std::ios::binary);
    src.open("test_data/newsflash_2_0_0.png", std::ios::binary);

    BOOST_REQUIRE(ref.is_open());
    BOOST_REQUIRE(src.is_open());

    std::vector<char> png;
    std::copy(std::istreambuf_iterator<char>(src), std::istreambuf_iterator<char>(), std::back_inserter(png));
    
    std::vector<char> data;
    std::copy(std::istreambuf_iterator<char>(ref), std::istreambuf_iterator<char>(), std::back_inserter(data));

    const std::string header("=ybegin line=128 size=16153 name=test.png\r\n");
    const std::string trailer("\r\n=yend size=16153 crc32=4a68571d\r\n");
    
    std::vector<char> yenc;

    std::copy(header.begin(), header.end(), std::back_inserter(yenc));
    yenc::encode(png.begin(), png.end(), std::back_inserter(yenc));
    std::copy(trailer.begin(), trailer.end(), back_inserter(yenc));

    BOOST_REQUIRE(yenc.size() == data.size());
    BOOST_REQUIRE(std::memcmp(&yenc[0], &data[0], yenc.size()) == 0);
}

/*
 * Synopsis: Verify that encoded data decodes back into origial data.
 *
 * Expected: Data matches.
 */
void test_decode_encode()
{
    char junk[1024 * sizeof(int)];
    
    std::srand(std::time(nullptr));

    for (int i=0; i<1024; ++i)
        ((int*)junk)[i] = rand();

    std::vector<char> yenc;
    std::vector<char> data;

    // encode...
    yenc::encode(junk, junk+sizeof(junk), std::back_inserter(yenc));
    
    // and decode back into binary
    yenc::decode(yenc.begin(), yenc.end(), std::back_inserter(data));
    
    BOOST_REQUIRE(data.size() == sizeof(junk));
    BOOST_REQUIRE(std::memcmp(&data[0], junk, sizeof(junk)) == 0);
    
}

int test_main(int, char* [])
{
    test_parsing();
    test_decoding();
    test_encoding();
    test_decode_encode();
    return 0;
}
