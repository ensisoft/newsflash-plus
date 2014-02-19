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
#include "../linebuffer.h"
#include "../bodyiter.h"


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

void test_parse_group()
{
    {
        const char* str = "asgljasgajsas";

        BOOST_REQUIRE(nntp::parse_group(str, std::strlen(str)).first == false);
    }

    {
        const char* str = "alt.binaries.foo.bar 4244 13222 y";

        const auto ret = nntp::parse_group(str, std::strlen(str));
        BOOST_REQUIRE(ret.first);
        BOOST_REQUIRE(ret.second.name == "alt.binaries.foo.bar");
        BOOST_REQUIRE(ret.second.last == "4244");
        BOOST_REQUIRE(ret.second.first == "13222");
        BOOST_REQUIRE(ret.second.posting == 'y');
    }

    {
        const char* str = "    asdgasgas.foobar      1222 111 y";
        const auto ret = nntp::parse_group(str, std::strlen(str));
        BOOST_REQUIRE(ret.first);
        BOOST_REQUIRE(ret.second.name == "asdgasgas.foobar");
        BOOST_REQUIRE(ret.second.last == "1222");
        BOOST_REQUIRE(ret.second.first == "111");
        BOOST_REQUIRE(ret.second.posting == 'y');
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

void test_find_filename()
{
    struct test {
        const char* str;
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
        const auto& ret = nntp::find_filename(it.str, std::strlen(it.str));

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
}

void test_bodyiter()
{
    // if a textual body response constains a double as the first character on a 
    // new line the dot is doubled. (".\r\n" indicates the end of transmission)
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
}

int test_main(int, char* [])
{
    test_parse_overview();
    test_parse_date();
    test_parse_group();
    test_date();
    test_is_binary();
    test_find_filename();

    test_linebuffer();
    test_bodyiter();

    return 0;
   
}


