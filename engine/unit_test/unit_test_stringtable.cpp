// Copyright (c) 2010-2015 Sami Väisänen, Ensisoft 
//
// http://www.ensisoft.com
// 
// This software is copyrighted software. Unauthorized hacking, cracking, distribution
// and general assing around is prohibited.
// Redistribution and use in source and binary forms, with or without modification,
// without permission are prohibited.
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <boost/test/minimal.hpp>
#include <iostream>
#include "../stringtable.h"
#include "unit_test_common.h"

void test_basic()
{
    std::string str;
    for (std::size_t i=0; i<newsflash::stringtable::MAX_STRING_LEN; ++i)
    {
        const auto offset = i % ('Z' - 'A');
        str.push_back('A' + offset);
    }

    {
        struct test {
            const std::string str;
            newsflash::stringtable::key_t key;
        } tests[] = {
            {"x", 0},
            {"x", 0},
            {"jeesus ajaa mopolla", 0},
            {"123200001221", 0},
            {"|||121300/&.,,123¤#!\"``'''--||", 0},
            {"(0)", 0},
            {"{12}", 0},
            {str, 0}
        };

        newsflash::stringtable st;
        for (auto it = std::begin(tests); it != std::end(tests); ++it)
        {
            test& t = *it;
            t.key = st.add(t.str);
        }        

         for (auto it = std::begin(tests); it != std::end(tests); ++it)
         {
            test& t = *it;
            const auto str = st.get(t.key);
            BOOST_REQUIRE(str == t.str);
        }   
    }

    // test some collisions
    {
        newsflash::stringtable st;
        const auto k1 = st.add("1 foobar");
        const auto k2 = st.add("2 foobar");
        BOOST_REQUIRE(st.get(k1) == "1 foobar");
        BOOST_REQUIRE(st.get(k2) == "2 foobar");

        const auto k3 = st.add("f 2121131121 b");
        const auto k4 = st.add("x 1222 y");
        const auto k5 = st.add("x 0 y");
        const auto k6 = st.add("f asglaasda111 y");
        BOOST_REQUIRE(st.get(k3) == "f 2121131121 b");
        BOOST_REQUIRE(st.get(k4) == "x 1222 y");
        BOOST_REQUIRE(st.get(k5) == "x 0 y");
        BOOST_REQUIRE(st.get(k6) == "f asglaasda111 y");
    }

}

void test_compress()
{
    struct test {
        const char* str;
        newsflash::stringtable::key_t key;
    } tests[] = {
        {"60.Minutes.2007.09.30.PDTV.SoS.part16.rar - [18/38] - yEnc (01/69)", 0},
        {"60.Minutes.2007.09.30.PDTV.SoS.part17.rar - [18/38] - yEnc (01/69)", 0},
        {"60.Minutes.2007.09.30.PDTV.SoS.part17.rar - [18/38] - yEnc (01/69)", 0},
        {"60.Minutes.2007.10.30.PDTV.SoS.part17.rar - [18/38] - yEnc (01/64)", 0},
        {"60.Minutes.2007.09.30.PDTV.SoS.part17.rar - [18/38] - yEnc (01/68)", 0},
        {"60.Minutes.3123.09.30.PDTV.SoS.part17.rar - [18/38] - yEnc (01/67)", 0},
        {"#A.B.MM @  EFNet Presents: REQ 40092 - Seinfeld.S09.DVDRip.XviD-SiNK - 482/520 - sink-seinfeld.s09e21e22.r23 (2/9)", 0},
        {"#A.B.MM @  EFNet Presents: REQ 40092 - Seinfeld.S09.DVDRip.XviD-SiNK - 482/520 - sink-seinfeld.s09e21e22.r23 (3/9)", 0},
        {"#A.B.MM @  EFNet Presents: REQ 40092 - Seinfeld.S09.DVDRip.XviD-SiNK - 482/520 - sink-seinfeld.s09e21e22.r23 (9/9)", 0},
        {"#A.B.MM @  EFNet Presents: REQ 40092 - Seinfeld.S09.DVDRip.XviD-SiNK - 482/520 - sink-seinfeld.s09e21e22.r23 (4/9)", 0},
        {"#A.B.MM @  EFNet Presents: REQ 40092 - Seinfeld.S09.DVDRip.XviD-SiNK - 482/520 - sink-seinfeld.s09e21e22.r23 (1/9)", 0},
        {"#A.B.MM @  EFNet Presents: REQ 40092 - Seinfeld.S09.DVDRip.XviD-SiNK - 482/520 - sink-seinfeld.s09e21e22.r23 (8/9)", 0},
        {"#A.B.MM @  EFNet Presents: REQ 40092 - Seinfeld.S09.DVDRip.XviD-SiNK - 482/520 - sink-seinfeld.s09e21e22.r23 (7/9)", 0},
        {"#A.B.MM @  EFNet Presents: REQ 40092 - Seinfeld.S09.DVDRip.XviD-SiNK - 482/520 - sink-seinfeld.s09e21e22.r23 (5/9)", 0},
        {"#A.B.MM @  EFNet Presents: REQ 40092 - Seinfeld.S09.DVDRip.XviD-SiNK - 482/520 - sink-seinfeld.s09e21e22.r23 (6/9)", 0},
        {"The_Magic_Stick_DVDrip.part9.rar (004/100)", 0},
        {"The_Magic_Stick_DVDrip.part9.rar (005/100)", 0},
        {"The_Magic_Stick_DVDrip.part9.rar (099/100)", 0},
        {"The_Magic_Stick_DVDrip.part10.rar (11/15)", 0},
        {"The_Magic_Stick_DVDrip.part10.rar (12/15)", 0}
    };

    newsflash::stringtable st;
    for (auto it = std::begin(tests); it != std::end(tests); ++it)
    {
        test& t = *it;
        t.key = st.add(t.str);
    }

    for (auto it = std::begin(tests); it != std::end(tests); ++it)
    {
        test& t = *it;
        const auto str = st.get(t.key);
        BOOST_REQUIRE(str == t.str);
    }

#ifdef NEWSFLASH_DEBUG
    const auto sizes = st.size_info();
    std::cout << "Raw space: " << sizes.first << " bytes\n";
    std::cout << "Table size: " << sizes.second << " bytes\n";
    std::cout << (double)sizes.second / (double)sizes.first << std::endl;
#endif    
}


int test_main(int, char*[])
{
    test_basic();
    test_compress();

    return 0;
}