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

#include <boost/test/minimal.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include "../nntp.h"


void test_parse_overview()
{
    const auto& REQUIRE = [](const nntp::overview::field& field, const char* expected)
    {
        if (expected)
        {
            BOOST_REQUIRE(field.len == std::strlen(expected));
            BOOST_REQUIRE(!std::strncmp(field.start, expected, field.len));
        }
        else
        {
            BOOST_REQUIRE(field.len == 0);
            BOOST_REQUIRE(field.start == nullptr);
        }
    };

    {
        // full overview without omitted fields
        const char* str = "123456\tsubject\tauthor\t22-4-2007\t<messageid>\treferences\t12223\t50\t";

        const auto& ret = nntp::parse_overview(str, std::strlen(str));
        BOOST_REQUIRE(ret.first);

        REQUIRE(ret.second.number, "123456");
        REQUIRE(ret.second.subject, "subject");
        REQUIRE(ret.second.author, "author");
        REQUIRE(ret.second.date, "22-4-2007");
        REQUIRE(ret.second.messageid, "<messageid>");
        REQUIRE(ret.second.references, "references");
        REQUIRE(ret.second.bytecount, "12223");
        REQUIRE(ret.second.linecount, "50");
    }

    {
        // full overview with omitted fields (date, references)
        const char* str = "123456\tsubject\tauthor\t\t<messageid>\t\t12223\t50\t";
        const auto& ret = nntp::parse_overview(str, std::strlen(str));
        BOOST_REQUIRE(ret.first);

        REQUIRE(ret.second.number, "123456");
        REQUIRE(ret.second.subject, "subject");
        REQUIRE(ret.second.author, "author");
        REQUIRE(ret.second.date, nullptr);
        REQUIRE(ret.second.messageid, "<messageid>");
        REQUIRE(ret.second.references, nullptr);
        REQUIRE(ret.second.bytecount, "12223");
        REQUIRE(ret.second.linecount, "50");        
    }

    {
        // full overview with some junk at the start of subject line
        const char* str = "123456\t!!!!!  subject\tauthor\t22-4-2007\t<messageid>\treferences\t12223\t50\t";

        const auto& ret = nntp::parse_overview(str, std::strlen(str));
        BOOST_REQUIRE(ret.first);

        REQUIRE(ret.second.number, "123456");
        REQUIRE(ret.second.subject, "subject");
        REQUIRE(ret.second.author, "author");
        REQUIRE(ret.second.date, "22-4-2007");
        REQUIRE(ret.second.messageid, "<messageid>");
        REQUIRE(ret.second.references, "references");
        REQUIRE(ret.second.bytecount, "12223");
        REQUIRE(ret.second.linecount, "50");
    }

    {
        // full overview with only one field
        const char* str = "\t\t\t22-4-2007\t\t\t\t\t";

        const auto& ret = nntp::parse_overview(str, std::strlen(str));
        BOOST_REQUIRE(ret.first);

        REQUIRE(ret.second.number, nullptr);
        REQUIRE(ret.second.subject, nullptr);
        REQUIRE(ret.second.author, nullptr);
        REQUIRE(ret.second.date, "22-4-2007");
        REQUIRE(ret.second.messageid, nullptr);
        REQUIRE(ret.second.references, nullptr);
        REQUIRE(ret.second.bytecount, nullptr);
        REQUIRE(ret.second.linecount, nullptr);
    }

    {
        // broken data (one field missing)
        const char* str = "123456\tsubject\tauthor\t\t<messageid>\t\t12223\t";        
        BOOST_REQUIRE(nntp::parse_overview(str, std::strlen(str)).first == false);
    }

    {
        // broken data
        const char* str = "asaafsfljasf\t";
        BOOST_REQUIRE(nntp::parse_overview(str, std::strlen(str)).first == false);
    }

    {
        // zero length
        const char* str = "foobar\t";
        BOOST_REQUIRE(nntp::parse_overview(str, 0).first == false);
    }
}

void test_parse_date()
{
    {
        const char* str = "Thu, 26 Jul 2007 19:44:13 -0500";

        const auto& ret = nntp::parse_date(str, std::strlen(str));
        BOOST_REQUIRE(ret.first);

        BOOST_REQUIRE(ret.second.day == 26);
        BOOST_REQUIRE(ret.second.month == "Jul");
        BOOST_REQUIRE(ret.second.year  == 2007);
        BOOST_REQUIRE(ret.second.hours  == 19);
        BOOST_REQUIRE(ret.second.minutes == 44);
        BOOST_REQUIRE(ret.second.seconds == 13);
        BOOST_REQUIRE(ret.second.tzoffset == -500);
    }

    {

        const char* str = "Wed, 30 Jun 2010 13:24:51 +0000 (UTC)";

        const auto& ret = nntp::parse_date(str, std::strlen(str));
        BOOST_REQUIRE(ret.first);
        BOOST_REQUIRE(ret.second.day == 30);
        BOOST_REQUIRE(ret.second.month == "Jun");
        BOOST_REQUIRE(ret.second.year  == 2010);
        BOOST_REQUIRE(ret.second.hours  == 13);
        BOOST_REQUIRE(ret.second.minutes == 24);
        BOOST_REQUIRE(ret.second.seconds == 51);
        BOOST_REQUIRE(ret.second.tzoffset == 0);
        BOOST_REQUIRE(ret.second.tz == "UTC");
    }

    {

        const char* str = "Thu, 26 Jul  2007  19:44:13";

        const auto& ret = nntp::parse_date(str, std::strlen(str));
        BOOST_REQUIRE(ret.first);
        BOOST_REQUIRE(ret.second.day == 26);
        BOOST_REQUIRE(ret.second.month == "Jul");
        BOOST_REQUIRE(ret.second.year  == 2007);
        BOOST_REQUIRE(ret.second.hours  == 19);
        BOOST_REQUIRE(ret.second.minutes == 44);
        BOOST_REQUIRE(ret.second.seconds == 13);
        BOOST_REQUIRE(ret.second.tzoffset == 0);
    }


    {

        const char* str = "29 Jul 2007 11:25:26 GMT";

        const auto& ret = nntp::parse_date(str, std::strlen(str));
        BOOST_REQUIRE(ret.first);
        BOOST_REQUIRE(ret.second.day == 29);
        BOOST_REQUIRE(ret.second.month == "Jul");
        BOOST_REQUIRE(ret.second.year == 2007);
        BOOST_REQUIRE(ret.second.hours == 11);
        BOOST_REQUIRE(ret.second.minutes == 25);
        BOOST_REQUIRE(ret.second.seconds == 26);
        BOOST_REQUIRE(ret.second.tzoffset == 0);
        BOOST_REQUIRE(ret.second.tz == "GMT");
    }

    {

        const char* str = "29  Jul  2007 11:25:26 foobar";

        const auto& ret = nntp::parse_date(str, std::strlen(str));
        BOOST_REQUIRE(ret.first);
        BOOST_REQUIRE(ret.second.day == 29);
        BOOST_REQUIRE(ret.second.month == "Jul");
        BOOST_REQUIRE(ret.second.year == 2007);
        BOOST_REQUIRE(ret.second.hours == 11);
        BOOST_REQUIRE(ret.second.minutes == 25);
        BOOST_REQUIRE(ret.second.seconds == 26);
        BOOST_REQUIRE(ret.second.tzoffset == 0);
        BOOST_REQUIRE(ret.second.tz == "foobar");

    }


    {
        const char* str = "29 Foo 12111aaaa";

        const auto& ret = nntp::parse_date(str, std::strlen(str));
        BOOST_REQUIRE(ret.first == false);
    }

    {
        const char* str = "Thu, 26 Jul  2007  1944 13";

        const auto& ret = nntp::parse_date(str, std::strlen(str));
        BOOST_REQUIRE(ret.first == false);
    }

    {
        const char* str = "Thu, 26 Jul  2007  19:44:13";

        const auto& ret = nntp::parse_date(str, 0);
        BOOST_REQUIRE(ret.first == false);
    }
}

void test_date()
{
    {
        nntp::date date;
        date.day = 26;
        date.month = "Feb";
        date.year  = 2005;
        date.hours = 23;
        date.minutes = 15;
        date.seconds = 14;
        date.tzoffset = 0;

        const auto t1 = nntp::timevalue(date);
        std::cout << "t1: " << std::ctime(&t1) << std::endl;

        date.day = 27;
        BOOST_REQUIRE(nntp::timevalue(date) > t1);

        date.day = 26;
        date.month = "Jan";
        BOOST_REQUIRE(nntp::timevalue(date) < t1);

        date.month = "Feb";
        date.year = 2006;
        BOOST_REQUIRE(nntp::timevalue(date) > t1);

        date.year = 2005;
        date.hours = 22;
        BOOST_REQUIRE(nntp::timevalue(date) < t1);

        date.hours = 23;
        date.minutes = 16;
        BOOST_REQUIRE(nntp::timevalue(date) > t1);

        date.minutes = 15;
        date.seconds = 13;
        BOOST_REQUIRE(nntp::timevalue(date) < t1);
    }

    {
        nntp::date date;
        date.day = 1;
        date.month = "Jan";
        date.year = 2000;
        date.hours = 2;
        date.minutes = 0;
        date.seconds = 0;
        date.tzoffset = 0;

        const auto t1 = nntp::timevalue(date);
        std::cout << "t1: " << std::ctime(&t1) << std::endl;

        date.tzoffset = -200; // -2h 00 min
        BOOST_REQUIRE(nntp::timevalue(date) == t1 + 2 * 3600);
        date.tzoffset = -230; // -2h 30 min
        BOOST_REQUIRE(nntp::timevalue(date) == t1 + 2 * 3600 + 30 * 60);

        date.tzoffset = 200; // +2h 00min
        BOOST_REQUIRE(nntp::timevalue(date) == t1 - 2 * 3600);
        date.tzoffset = 230; // +2h 30min
        BOOST_REQUIRE(nntp::timevalue(date) == t1 - 2 * 3600 - 30 * 60);

        date.tzoffset = -200;
        date.hours = 0;
        BOOST_REQUIRE(nntp::timevalue(date) == t1);

        date.tzoffset = 200;
        date.hours = 4;
        BOOST_REQUIRE(nntp::timevalue(date) == t1);
    }
}

void test_is_binary()
{
    struct test {
        const char* str;
        bool expected_result;
    } tests[] = {
        {"foobar.rar", true},
        {"foobar.r01", true},
        {"foobar.r99", true},
        {"foobar.mp3", true},
        {"foobar.wmv", true},
        {"terminator 2 - judgement day.001", true},
        {"terminator 2 - judgement day.001 enjoy!!", true},
        {"foobar.z45", true},
        {"foobar.mdf", true},
        {"\"Atlantis Rising Magazine #12.pdf\" [11/71] yEnc", true},
        {"\"foobar.rar [01/20] www.nzb-keke.com\"", true},
        {"1983 - 39 - Micael Semballo - Maniac .mp3\"The Best of Von 75er", true},
        {"here is metallica-enter sandman.mp3By superposter!! yEnc (1/10)", true},
        {"molly123123.jpg(1/1)", true},
        {"(paska.jpg)(1/1)", true},
        {"baby brown - CC Soldier .mp3 Beaz in RnB & SouL (02/30)", true},
        {"schalke(9/9) $ yEnc (70/120)", true},
            
            
        {"foobar", false},
        {"foobar yEnc kkeekek", false},
        {".NET 2.0 WebResource", false},
        {"Please post DieHard.4.0:-thx", false},
        {"Please post something wild.1986", false},
        {"Query about GraphicsPath.Flatten() method", false},
        {"need help with .doc files", false},
        {".doc is a microsoft format", false},
        {".wmv", false},
        {"need help with .wmv files", false},
        {".wmv is a microsoft format", false},
        {"\"www.nzb-keke.com\"", false}
        //{"NFO Template.txt\"Mastermix-Vol-11-To-Vol-15--www.nzb-dogz.co.uk-- (1/1)", false}
    };

    for (const auto& it : tests)
    {
        const bool ret = nntp::is_binary_post(it.str, std::strlen(it.str));
        BOOST_REQUIRE(ret == it.expected_result);
    }
}





// void test12()
// {
//     const char* str1 = "a subject line";
//     const char* str2 = "a subJECT line";
//     const char* str3 = "foobar keke gu";
//     const char* str4 = "Metallica - 02 - Enter sandman (1/15).mp3";
//     const char* str5 = "Metallica - 02 - Enter sandman (2/15).mp3";
//     const char* str6 = "[foobar]";
//     const char* str7 = "[doodar]";
//     const char* str8 = "some weird letters öööäää,,,<|^^ÅÅ";
//     const char* str9 = "foobar keke (01/50)";
//     const char* strA = "foobar keke (01/xy)";


//     BOOST_REQUIRE(test::strcmp(str1, str2) ==  0); // IGNORE CASE
//     BOOST_REQUIRE(test::strcmp(str1, str3) == -1);
//     BOOST_REQUIRE(test::strcmp(str4, str5) ==  0);
//     BOOST_REQUIRE(test::strcmp(str6, str7) ==  1);
//     BOOST_REQUIRE(test::strcmp(str8, str8) ==  0);
//     BOOST_REQUIRE(test::strcmp(str9, strA) !=  0); 


//     BOOST_REQUIRE(test::strcmp("abba", "aBBA") == 0);
//     BOOST_REQUIRE(test::strcmp("Abba", "abba") == 0);
//     BOOST_REQUIRE(test::strcmp("abba", "ABBA") == 0);

//     BOOST_REQUIRE(test::strcmp("Abba", "Babba") == -1);
//     BOOST_REQUIRE(test::strcmp("abel", "Babel") == -1);
//     BOOST_REQUIRE(test::strcmp("Babel", "abel") == 1);

//     BOOST_REQUIRE(test::strcmp("aaa", "aaaaa") == -1);
//     BOOST_REQUIRE(test::strcmp("aaaaa", "aaa") == 1);
    
//     BOOST_REQUIRE(test::strcmp("ää", "öö") == -1);
//     BOOST_REQUIRE(test::strcmp("ÖÖ", "öö") == 0);

//     {
//         const char* str1 = "\"01 - Kreator - Extreme Aggressions.mp3\" (3/20) yEnc";
//         const char* str2 = "\"01 - Kreator - Extreme Aggressions.mp3\" (10/20) yEnc";

//         const char* str3 = "foobar (2/145).mp3";
//         const char* str4 = "foobar (100/145).mp3";

//         BOOST_REQUIRE(test::strcmp(str1, str2) == 0);
//         BOOST_REQUIRE(test::strcmp(str3, str4) == 0);

//     }
//     {
//         const char* str1 = "foobar (34/45)";
//         const char* str2 = "foobar (34/70)"; // different parts! 
        
//         BOOST_REQUIRE(test::strcmp(str1, str2) == -1);
        
//     }

//     { 
//         const char* str1 = "foobar (1234)";
//         const char* str2 = "foobar (4321)";
        
//         BOOST_REQUIRE(test::strcmp(str1, str2) == -1);
//         BOOST_REQUIRE(test::strcmp(str2, str1) == 1);

//     }
    
//     //BOOST_REQUIRE(test::strcmp("0001234assa", "abcdef") == ignore_case_strcmp("0001234assa", "abcdef"));
//     //BOOST_REQUIRE(test::strcmp("z1z2abc3", "[[101abcd") == ignore_case_strcmp("z1z2abc3", "[[101abcd"));
// }

// void test13()
// {
//     const char* str1 = "foobar there's no part count here";
//     const char* str2 = "a part count yEnc (1/50)";
//     const char* str3 = "another part count yEnc (0001/0048)";
//     const char* str4 = "looks like a part count yEnc  (14) but isnt";
//     const char* str5 = "foobar yEnc   (1/9)";
//     const char* str6 = " foo (1/10) keke";
//     const char* str7 = "lets post some pictures (2) foobar.jpg(1/1) ";
//     const char* str8 = "(45/46) foobar";
//     const char* str9 = "Killrape - Corrosive Birth (2010)(5/7) - \"Killrape - Corrosive Birth (2010).part3.rar\" - 71,70 MB yEnc (4/24)";

//     int i;
//     const char* str;
//     nntp_part p;

//     str = nntp_find_part_count(str1, strlen(str1), i);
//     BOOST_REQUIRE(str == NULL);
    
//     str = nntp_find_part_count(str2, strlen(str2), i);
//     BOOST_REQUIRE(str != NULL);
//     BOOST_REQUIRE(strncmp(str, "(1/50)", 6) == 0);
//     BOOST_REQUIRE(nntp_parse_part_count(str, i, p));
//     BOOST_REQUIRE(p.count = 1 && p.max == 50);


//     str = nntp_find_part_count(str3, strlen(str3), i);
//     BOOST_REQUIRE(str != NULL);
//     BOOST_REQUIRE(strncmp(str, "(0001/0048)", 11) == 0);
//     BOOST_REQUIRE(nntp_parse_part_count(str, i, p));
//     BOOST_REQUIRE(p.count == 1 && p.max == 48);


//     str = nntp_find_part_count(str4, strlen(str4), i);
//     BOOST_REQUIRE(str == NULL);


//     str = nntp_find_part_count(str5, strlen(str5), i);
//     BOOST_REQUIRE(str != NULL);
//     BOOST_REQUIRE(strncmp(str, "(1/9)", 5) == 0);
//     BOOST_REQUIRE(nntp_parse_part_count(str, i, p));
//     BOOST_REQUIRE(p.count == 1 && p.max == 9);
    

//     str = nntp_find_part_count(str6, strlen(str6), i);
//     BOOST_REQUIRE(str != NULL);
//     BOOST_REQUIRE(strncmp(str, "(1/10)", 6) == 0);
//     BOOST_REQUIRE(nntp_parse_part_count(str, i, p));
//     BOOST_REQUIRE(p.count == 1 && p.max == 10);
    
//     str = nntp_find_part_count(str7, strlen(str7), i);
//     BOOST_REQUIRE(str != NULL);
//     BOOST_REQUIRE(strncmp(str, "(1/1)", 5) == 0);

//     str = nntp_find_part_count(str8, strlen(str8), i);
//     BOOST_REQUIRE(str == NULL);
    
//     str = nntp_find_part_count(str9, strlen(str9), i);
//     BOOST_REQUIRE(str != NULL);
//     BOOST_REQUIRE(strncmp(str, "(4/24)", 6) == 0);
// }

// #include <boost/lexical_cast.hpp>
// #include <cstdlib>

// void test14_0()
// {
//     BOOST_REQUIRE(nntp_to_int<int>("1234",  4) == 1234);
//     BOOST_REQUIRE(nntp_to_int<int>("50000", 5) == 50000);
//     BOOST_REQUIRE(nntp_to_int<int>("0", 1)     == 0);
//     BOOST_REQUIRE(nntp_to_int<int>("1", 1)     == 1);
//     BOOST_REQUIRE(nntp_to_int<int>("0003", 4)  == 3);



//     bool overflow = false;
//     BOOST_REQUIRE(nntp_to_int<int>("0", 1, overflow) == 0);    
//     BOOST_REQUIRE(overflow == false);
//     BOOST_REQUIRE(nntp_to_int<int>("1234", 4, overflow) == 1234);
//     BOOST_REQUIRE(overflow == false);
//     BOOST_REQUIRE(nntp_to_int<int>("50000", 5, overflow) == 50000);
//     BOOST_REQUIRE(overflow == false);
//     BOOST_REQUIRE(nntp_to_int<int>("637131", 6, overflow) == 637131);
//     BOOST_REQUIRE(overflow == false);
//     BOOST_REQUIRE(nntp_to_int<int>("7342352", 7, overflow) == 7342352);
//     BOOST_REQUIRE(overflow == false);
//     BOOST_REQUIRE(nntp_to_int<int>("87811234", 8, overflow) == 87811234);
//     BOOST_REQUIRE(overflow == false);
//     BOOST_REQUIRE(nntp_to_int<int>("534235256", 9, overflow) == 534235256);
//     BOOST_REQUIRE(overflow == false);
//     BOOST_REQUIRE(nntp_to_int<int>("2875543543", 10, overflow) == 0);
//     BOOST_REQUIRE(overflow == true);

//     BOOST_REQUIRE(nntp_to_int<short>("81023", 5, overflow) == 0);
//     BOOST_REQUIRE(overflow == true);

//     BOOST_REQUIRE(nntp_to_int<int>("9147483648", 10, overflow) == 0);
//     BOOST_REQUIRE(overflow == true);

//     BOOST_REQUIRE(nntp_to_int<int>("000123", 6, overflow) == 123);
//     BOOST_REQUIRE(overflow == false);

//     BOOST_REQUIRE(nntp_to_int<boost::uint32_t>("116812835", 9, overflow) == 116812835);
//     BOOST_REQUIRE(overflow == false);

//     {
//         BOOST_REQUIRE(nntp_to_int<unsigned char>("254", 3, overflow) == 254);
//         BOOST_REQUIRE(overflow == false);
    
//         BOOST_REQUIRE(nntp_to_int<unsigned char>("255", 3, overflow) == 255);
//         BOOST_REQUIRE(overflow == false);

//         BOOST_REQUIRE(nntp_to_int<unsigned char>("256", 3, overflow) == 0);
//         BOOST_REQUIRE(overflow == true);

//     }
    
//     {
//         bool overflow;
//         string tmp;
//         unsigned int max = std::numeric_limits<int>::max();
        
//         max -= 1;
//         tmp  = lexical_cast<string>(max);
//         BOOST_REQUIRE(nntp_to_int<int>(tmp.c_str(), tmp.size(), overflow) == (int)max);
//         BOOST_REQUIRE(overflow == false);
        
//         max += 1;
//         tmp  = lexical_cast<string>(max);
//         BOOST_REQUIRE(nntp_to_int<int>(tmp.c_str(), tmp.size(), overflow) == (int)max);
//         BOOST_REQUIRE(overflow == false);
        
//         max += 1;
//         tmp  = lexical_cast<string>(max);
//         BOOST_REQUIRE(nntp_to_int<int>(tmp.c_str(), tmp.size(), overflow) == 0);
//         BOOST_REQUIRE(overflow == true);
//     }
    
// }

// /*
//  * Synopsis: measure the performance of string to int conversions
//  *
//  */
// void test14()
// {

//     cout << "\"atoi\" times:\n";

//     {
//         ms_t now = sys_ms();
        
//         for (int i=0; i<2000000; ++i)
//             nntp_to_int<int>("123456", 6);

//         ms_t end  = sys_ms();
//         ms_t time = end - now;
        
//         cout << "time1: " << time << "ms\tnntp_to_int()\n";
//     }

//     {
//         ms_t now = sys_ms();
        
//         bool overflow;
//         for (int i=0; i<2000000; ++i)
//             nntp_to_int<int>("123456", 6, overflow);

//         ms_t end  = sys_ms();
//         ms_t time = end - now;
        
//         cout << "time2: " << time << "ms\tnntp_to_int(overflow)\n";
//     }


//     {
//         ms_t now = sys_ms();
        
//         for (int i=0; i<2000000; ++i)
//         {
//             int ret = atoi("123456");
//             ret = 123;
//         }
        
//         ms_t end  = sys_ms();
//         ms_t time = end - now;
        
//         cout << "time3: " << time << "ms\tatoi()\n";
//     }

//     {
//         ms_t now = sys_ms();
        
//         for (int i=0; i<2000000; ++i)
//             boost::lexical_cast<int>("123456");

//         ms_t end  = sys_ms();
//         ms_t time = end - now;
        
//         cout << "time4: " << time << "ms\tlexical_cast<>\n";

//     }

// }

// void test15()
// {
//     cout << "strcmp times:\n";

//     const char* str = "Man is distinguished, not only by his reason, but by this singular passion from other animals, which is a lust of the mind... ";
//     {
//         ms_t now = sys_ms();
        
//         for (int i=0; i<2000000; ++i)
//         {
//             ignore_case_strcmp(str, str);
//         }
        
//         ms_t end  = sys_ms();
//         ms_t time = end - now;
//         cout << "time1: " << time << "ms\n";
//     }
//     {
//         ms_t now = sys_ms();
        
//         // stricmp can loop untill 0, so it doesnt need to 
//         // precalcute the lenght of the string all the time
//         int len = strlen(str);

//         for (int i=0; i<2000000; ++i)
//         {
//             nntp_strcmp(str, len, str, len);
//         }
        
//         ms_t end  = sys_ms();
//         ms_t time = end - now;
//         cout << "time2: " << time << "ms\n";
//     }
// }


void test_find_filename()
{
    struct test {
        const char* test;
        const char* expected;
    } tests[] = {
        {"fooobar music.mp3 foobar", "music.mp3"},
        {"foobar (music-file.mp3) bla blah", "music-file.mp3"},
        {"foobar [music file.mp3] bla blah", "music file.mp3"},
        {"foobar \"music file again.mp3\" blabla", "music file again.mp3"},
        {"music.mp3 bla blah blah", "music.mp3"},
        {"blah blah blah music.mp3", "music.mp3"},
        {"blah blah music.mp3\"", "blah blah music.mp3"},
        {"blah blah music.mp3]", "blah blah music.mp3"},

        {"01 Intro.mp3 (00/11) Paradise Lost yEnc", "Intro.mp3"},        
        {"01-Intro.mp3 (00/11) Paradise Lost yEnc", "01-Intro.mp3"},
        {"\"01 Intro.mp3\" (00/11) Paradise Lost yEnc", "01 Intro.mp3"},

        {"foo bar keke.mp3", "keke.mp3"},        
        {"   **** 1123nmnlullj.jpg", "1123nmnlullj.jpg"},
        {"\"(03) 03 - ALL HAIL TO THE QUEEN.mp3\" yEnc", "(03) 03 - ALL HAIL TO THE QUEEN.mp3"},
        {"(6/28) nGJ6001.JPG = foobar keke", "nGJ6001.JPG"},
        {"nGJ6001.JPG (6/28) = foobar", "nGJ6001.JPG"},
        {"foobar keke (6/28) nGJ6001.JPG", "nGJ6001.JPG"},
        {"\"Chaostage 1995.DAT\"", "Chaostage 1995.DAT"},
        {"\"music.mp3\" blaah blah", "music.mp3"},
        {"blah blah \"music.mp3", "music.mp3"},
        {"blah blah \"music.mp3\"", "music.mp3"},
        {"blah blah (music.mp3)", "music.mp3"},
        {"[music.mp3 (1/10)]", "music.mp3"},
        {"1.mp3", "1.mp3"},

        {"blah blah blah ----image.jpeg", "image.jpeg"},
        {"blah blah ----image.jpeg----", "image.jpeg"},

        {"", nullptr},
        {"foobar kekeke herp derp", nullptr},
        {".pdf foobar", nullptr},
        {"how to deal with .pdf files?", nullptr},
        {"anyone know about .pdf", nullptr},
        {"REQ...Metallica - Cunning stunts", nullptr},
        {"schalke(9/9) $ yEnc (70/121)", nullptr}
    };

    for (const auto& it : tests)
    {
        const auto& ret = nntp::find_filename(it.test, std::strlen(it.test));

        //std::cout << it.test << std::endl;

        if (it.expected)
        {
            BOOST_REQUIRE(ret.second == std::strlen(it.expected));
            BOOST_REQUIRE(!std::strncmp(it.expected, ret.first, ret.second));
        }
        else
        {
            BOOST_REQUIRE(ret.first == nullptr);
            BOOST_REQUIRE(ret.second == 0);
        }
    }
}

int test_main(int, char* [])
{
    test_parse_overview();
    test_parse_date();
    test_date();
    test_is_binary();
    test_find_filename();

    return 0;
   
}


