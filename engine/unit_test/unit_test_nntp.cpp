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

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <boost/test/minimal.hpp>
#  include <boost/lexical_cast.hpp>
#include "newsflash/warnpop.h"

#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstdlib>
#include <ctime>

#include "engine/nntp.h"

void test_reverse_iterator()
{
    const char* str = "jeesus ajaa mopolla";

    nntp::reverse_c_str_iterator beg(str + std::strlen(str) -1);
    nntp::reverse_c_str_iterator end(str - 1);
    std::string s;

    for (; beg != end; ++beg)
        s += *beg;

    BOOST_REQUIRE(s == "allopom aaja suseej");
}

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
        const char* str = "25534\t(cameltoe clips) [93/98] - \"CAMELTOE  CLIPS 6.vol076+32.PAR2\" yEnc (126/130)\tlekkrstrak@hotmail.com (cameltoelover)\tTue, 15 Feb 2011 20:45:19 +0100\t<6d263$4d5ad7cf$d979c5b1$19427@news.lightningusenet.com>\t\t931\t3052\tXref: news-big.astraweb.com alt.binaries.pictures.cameltoes:25534";

        const auto& ret = nntp::parse_overview(str, std::strlen(str));
        REQUIRE(ret.second.number, "25534");
        REQUIRE(ret.second.bytecount, "931");
        REQUIRE(ret.second.linecount, "3052");
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

void test_parse_part()
{
    auto ret = nntp::parse_part("Re: Shuffling");
    BOOST_REQUIRE(ret.first == false);

    ret = nntp::parse_part("New FREE-EBook: \"Evolved to Win\", by Moshe Sipper");
    BOOST_REQUIRE(ret.first == false);

    ret = nntp::parse_part("Girls have fun !!! babes-332.jpg (1/2)");
    BOOST_REQUIRE(ret.first == true);
    BOOST_REQUIRE(ret.second.numerator == 1);
    BOOST_REQUIRE(ret.second.denominator == 2);

    ret = nntp::parse_part("Girls have fun !!! foobar.avi [01/50]");
    BOOST_REQUIRE(ret.first == true);
    BOOST_REQUIRE(ret.second.numerator == 1);
    BOOST_REQUIRE(ret.second.denominator == 50);

    ret = nntp::parse_part("Girls have fun !!! [01/60] foobar.avi.001 (15/49)");
    BOOST_REQUIRE(ret.first == true);
    BOOST_REQUIRE(ret.second.numerator == 15);
    BOOST_REQUIRE(ret.second.denominator == 49);

    ret = nntp::parse_part("<Aokay>  Your ccde2010 Fills - 199|424 - yEnc - ccde_Klara_22.jpg (07/16)");
    BOOST_REQUIRE(ret.first == true);
    BOOST_REQUIRE(ret.second.numerator == 7);
    BOOST_REQUIRE(ret.second.denominator == 16);
}

void test_parse_group()
{
    {
        const char* str = "asgljasgajsas";

        BOOST_REQUIRE(nntp::parse_group_list_item(str, std::strlen(str)).first == false);
    }

    {
        const char* str = "alt.binaries.foo.bar 4244 13222 y";

        const auto ret = nntp::parse_group_list_item(str, std::strlen(str));
        BOOST_REQUIRE(ret.first);
        BOOST_REQUIRE(ret.second.name == "alt.binaries.foo.bar");
        BOOST_REQUIRE(ret.second.last == "4244");
        BOOST_REQUIRE(ret.second.first == "13222");
        BOOST_REQUIRE(ret.second.posting == 'y');
    }

    {
        const char* str = "    asdgasgas.foobar      1222 111 y";
        const auto ret = nntp::parse_group_list_item(str, std::strlen(str));
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
        // std::cout << "t1: " << std::ctime(&t1) << std::endl;

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
        // std::cout << "t1: " << std::ctime(&t1) << std::endl;

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
        std::string expected;
    } tests[] = {
        {"\"8zm6tvYNq2.part14.rar\" - 1.53GB <<< www.nfo-underground.xxx >>> yEnc (19/273)",
         "8zm6tvYNq2.part14.rar"},

        {"Darkstar - Heart of Darkstar - File 07 of 10 - Darkstar_Heart of Darkness_07_The Dream (Scene 2).mp3 (01/11)",
         "Darkstar_Heart of Darkness_07_The Dream (Scene 2).mp3"},

        {"[#scnzb@efnet][529762] Automata.2014.BRrip.x264.Ac3-MiLLENiUM [1/4] - \"Automata.2014.BRrip.x264.Ac3-MiLLENiUM.mkv\" yEnc (1/1513)",
            "Automata.2014.BRrip.x264.Ac3-MiLLENiUM.mkv"},

        {"Ip.Man.The.Final.Fight.2013.COMPLETE.BluRay-oOo - [1/7] - #34;Ip.Man.The.Final.Fight.2013.COMPLETE.BluRay-oOo.rar#34; yEnc (204/204)",
          "Ip.Man.The.Final.Fight.2013.COMPLETE.BluRay-oOo.rar"},

        {"Katatonia - Live Consternation - 04 Had to (Leave).mp3 Katatonia - Live Consternation Amsterdam [DM-320](16/19)",
          "04 Had to (Leave).mp3"},

        {"(Uncensored) Lovely Hina Otsuka scene 3.wmv.006 yEnc (22/30)",
          "(Uncensored) Lovely Hina Otsuka scene 3.wmv.006"},

        {"(Uncensored) black Gal Dance - Sakura Kiryu scene 1.wmv.vol110+110.PAR2 yEnc (01/30)",
          "(Uncensored) black Gal Dance - Sakura Kiryu scene 1.wmv.vol110+110.PAR2"},

        {"fooobar music.mp3 foobar", "fooobar music.mp3"},
        {"foobar (music-file.mp3) bla blah", "music-file.mp3"},
        {"foobar [music file.mp3] bla blah", "music file.mp3"},
        {"foobar \"music file again.mp3\" blabla", "music file again.mp3"},
        {"music.mp3 bla blah blah", "music.mp3"},
        {"blah blah blah music.mp3", "blah blah blah music.mp3"},
        {"blah blah music.mp3\"", "blah blah music.mp3"},
        {"blah blah music.mp3]", "blah blah music.mp3"},

        {"01 Intro.mp3 (00/11) Paradise Lost yEnc", "01 Intro.mp3"},
        {"01-Intro.mp3 (00/11) Paradise Lost yEnc", "01-Intro.mp3"},
        {"\"01 Intro.mp3\" (00/11) Paradise Lost yEnc", "01 Intro.mp3"},

        {"foo bar keke.mp3", "foo bar keke.mp3"},
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

        {".pdf", ""},

        {"", ""},
        {"foobar kekeke herp derp", ""},
        {".pdf foobar", ""},
        {"how to deal with .pdf files?", ""},
        {"anyone know about .pdf", ""},
        {"REQ...Metallica - Cunning stunts", ""},
        {"schalke(9/9) $ yEnc (70/121)", ""}
    };

    for (const auto& it : tests)
    {
        const auto& ret = nntp::find_filename(it.str, std::strlen(it.str));

        // std::cout << std::endl;
        // std::cout << "Test: " << it.str << std::endl;
        // std::cout << "Found: " << ret << std::endl;
        // std::cout << "Wanted: " << it.expected << std::endl;

        BOOST_REQUIRE(ret == it.expected);
    }


    BOOST_REQUIRE(nntp::find_filename(std::string("foobar [1/4] \"file.mp3\" yEnc (01/10)"), true) == "file.mp3");
    BOOST_REQUIRE(nntp::find_filename(std::string("foobar [1/4] \"file.mp3\" yEnc (01/10)"), false) == "file");

    BOOST_REQUIRE(nntp::find_filename(std::string("blah blah ---image.jpg---"), true) == "image.jpg");
    BOOST_REQUIRE(nntp::find_filename(std::string("blah blah ---image.jpg---"), false) == "image");


}

void test_find_response()
{
    BOOST_REQUIRE(nntp::find_response("", 0) == 0);
    BOOST_REQUIRE(nntp::find_response("adsgas", 6) == 0);
    BOOST_REQUIRE(nntp::find_response("\r\n", 2) == 2);
    BOOST_REQUIRE(nntp::find_response("foo\r\n", 5) == 5);
    BOOST_REQUIRE(nntp::find_response("foo\r\nfoo", 5) == 5);
}



void test_find_body()
{

    BOOST_REQUIRE(nntp::find_body("", 0) == 0);
    BOOST_REQUIRE(nntp::find_body("asgas", 5) == 0);
    BOOST_REQUIRE(nntp::find_body(".\r\n", 3) == 3);
    BOOST_REQUIRE(nntp::find_body("foobar.\r\n", 9) == 0);
    BOOST_REQUIRE(nntp::find_body("keke\r\n.\r\n", 9) == 9);
    BOOST_REQUIRE(nntp::find_body("foo\r\nkeke\r\n.\r\n", strlen("foo\r\nkeke\r\n.\r\n")) == strlen("foo\r\nkeke\r\n.\r\n"));
    BOOST_REQUIRE(nntp::find_body("foo.\r\nkeke\r\n.\r\n", strlen("foo.\r\nkeke\r\n.\r\n")) == strlen("foo.\r\nkeke\r\n.\r\n"));

    {
        const char* str =
         "On 2/23/2013 10:55 AM, James Silverton wrote:\r\n"
         "> On 2/23/2013 10:08 AM, RussCA wrote:\r\n"
         ">> On 2/23/2013 9:01 AM, John wrote:\r\n"
         ">>> On 2/22/2013 8:58 PM, RussCA wrote:\r\n"
         ">>>> I just printed a message from this newsgroup.\r\n"
         ">>>>\r\n"
         ">>>> I am using the addon \"TT DeepDark 4.1\", so that my text is white and\r\n"
         ">>>> background is black(ish).\r\n"
         ">>>>\r\n"
         ">>>> The printed output is white text on a black background.  Quite a waste\r\n"
         ">>>> of black ink.  Wouldn't it be better if the printing process knew\r\n"
         ">>>> something about printing what the message colors are, rather than what\r\n"
         ">>>> the displayed message looks like?\r\n"
         ">>>\r\n"
         ">>>\r\n"
         ">>> When I want to print text only I transfer the text to a text (only)\r\n"
         ">>> editor. Hi-light the text, Ctrl<C>, and Ctrl<V> into the editor and\r\n"
         ">>> print from the editor --- sure saves on ink ;)\r\n"
         ">>>\r\n"
         ">>> John\r\n"
         ">> John,\r\n"
         ">> I know.  That's what I usually do when there is something specific I\r\n"
         ">> want to save from the message.  If I'm in a hurry, though, it's often\r\n"
         ">> easier to simply print.  I guess I'll have to simply abandon printing\r\n"
         ">> from TB and possible FF as well.  Unfortunately, some web pages don't\r\n"
         ">> let you select and copy.\r\n"
         ">>\r\n"
         ">>\r\n"
         "> I think it is easier to copy text only with programs like PureText\r\n"
         "> rather than carefully high lighting. I have defined a key to strip\r\n"
         "> formatting and copy. That is an available option and I have chosen\r\n"
         "> CTRL-0. The method can be used with both full text editors like Word and\r\n"
         "> text only editors.\r\n"
         "\r\n"
         "\r\n"
         "\r\n"
         "That looks interesting. Does it address \"some web pages don't let you\r\n"
         "select and copy\" problem RussCA has experienced? The PureText web page\r\n"
         "doesn't seem to address that issue.\r\n"
         "\r\n"
         "Just curious,\r\n"
         "John\r\n"
         ".\r\n";

        const std::size_t len = nntp::find_body(str, std::strlen(str));

        BOOST_REQUIRE(len == std::strlen(str));
    }
}

#define REQUIRE_EXCEPTION(x) \
    try { \
        x; \
        BOOST_FAIL("exception was expected"); \
    } \
    catch(const std::exception& e) {}

void test_scan_response()
{
    {
        int value;
        std::string foo, bar;
        BOOST_REQUIRE(nntp::scan_response({123}, "123 234254 foo bar",
            strlen("123 234254 foo bar"), value, foo, bar) == 123);
        BOOST_REQUIRE(value == 234254);
        BOOST_REQUIRE(foo == "foo");
        BOOST_REQUIRE(bar == "bar");

    }

    {
        BOOST_REQUIRE(nntp::scan_response({125}, "125 foobar jeje\r\n",
            strlen("125 foobar jeje\r\n")) == 125);
    }

    {
        int value;
        nntp::trailing_comment cmt;
        BOOST_REQUIRE(nntp::scan_response({233, 200, 405}, "233 12344 welcome posting allowed",
            strlen("233 12344 welcome posting allowed"), value, cmt) == 233);

        BOOST_REQUIRE(value == 12344);
        BOOST_REQUIRE(cmt.str == "welcome posting allowed");
    }

    int value;
    std::string str;

    REQUIRE_EXCEPTION(nntp::scan_response({100}, "asdgasgas", strlen("asdgasgas")));
    REQUIRE_EXCEPTION(nntp::scan_response({100}, "123 foobar keke", strlen("123 foobar keke")));
    REQUIRE_EXCEPTION(nntp::scan_response({100}, "100 foobar keke", strlen("100 foobar keke"), value));
}


void test_to_int()
{
    using namespace nntp;

    bool overflow = false;

    BOOST_REQUIRE(to_int<int>("1234",  4) == 1234);
    BOOST_REQUIRE(to_int<int>("50000", 5) == 50000);
    BOOST_REQUIRE(to_int<int>("0", 1)     == 0);
    BOOST_REQUIRE(to_int<int>("1", 1)     == 1);
    BOOST_REQUIRE(to_int<int>("0003", 4)  == 3);
    BOOST_REQUIRE(to_int<int>("0", 1, overflow) == 0);
    BOOST_REQUIRE(overflow == false);
    BOOST_REQUIRE(to_int<int>("1234", 4, overflow) == 1234);
    BOOST_REQUIRE(overflow == false);
    BOOST_REQUIRE(to_int<int>("50000", 5, overflow) == 50000);
    BOOST_REQUIRE(overflow == false);
    BOOST_REQUIRE(to_int<int>("637131", 6, overflow) == 637131);
    BOOST_REQUIRE(overflow == false);
    BOOST_REQUIRE(to_int<int>("7342352", 7, overflow) == 7342352);
    BOOST_REQUIRE(overflow == false);
    BOOST_REQUIRE(to_int<int>("87811234", 8, overflow) == 87811234);
    BOOST_REQUIRE(overflow == false);
    BOOST_REQUIRE(to_int<int>("534235256", 9, overflow) == 534235256);
    BOOST_REQUIRE(overflow == false);

    BOOST_REQUIRE(to_int<int>("2875543543", 10, overflow) == 0);
    BOOST_REQUIRE(overflow == true);
    BOOST_REQUIRE(to_int<short>("81023", 5, overflow) == 0);
    BOOST_REQUIRE(overflow == true);
    BOOST_REQUIRE(to_int<int>("9147483648", 10, overflow) == 0);
    BOOST_REQUIRE(overflow == true);
    BOOST_REQUIRE(to_int<int>("000123", 6, overflow) == 123);
    BOOST_REQUIRE(overflow == false);
    BOOST_REQUIRE(to_int<std::uint32_t>("116812835", 9, overflow) == 116812835);
    BOOST_REQUIRE(overflow == false);

}

void test_strcmp()
{
    std::string str1 = "a subject line";
    std::string str2 = "a subJECT line";
    std::string str3 = "foobar keke gu";
    std::string str4 = "Metallica - 02 - Enter sandman (1/15).mp3";
    std::string str5 = "Metallica - 02 - Enter sandman (2/15).mp3";
    std::string str6 = "[foobar]";
    std::string str7 = "[doodar]";
    std::string str8 = "some weird letters öööäää,,,<|^^ÅÅ";
    std::string str9 = "foobar keke (01/50)";
    std::string strA = "foobar keke (01/xy)";

    BOOST_REQUIRE(nntp::strcmp(str1, str1) == true);
    BOOST_REQUIRE(nntp::strcmp(str1, str2) == false); // IGNORE CASE
    BOOST_REQUIRE(nntp::strcmp(str1, str3) == false);
    BOOST_REQUIRE(nntp::strcmp(str4, str4) == true);
    BOOST_REQUIRE(nntp::strcmp(str4, str4) == true);
    BOOST_REQUIRE(nntp::strcmp(str6, str6) == true);
    BOOST_REQUIRE(nntp::strcmp(str8, str8) == true);

    BOOST_REQUIRE(nntp::strcmp(
        "[ TOWN ]-[ www.town.ag ]-[ partner of www.ssl-news.info ] [15/32] - \"6nf3wQL3uc.part14.rar\" - 2,56 GB yEnc (71/273)",
        "[ TOWN ]-[ www.town.ag ]-[ partner of www.ssl-news.info ] [15/32] - \"6nf3wQL3uc.part14.rar\" - 2,56 GB yEnc (42/273)"));

    BOOST_REQUIRE(nntp::strcmp(
        "[ TOWN ]-[ www.town.ag ]-[ partner of www.ssl-news.info ] [15/32] - \"6nf3wQL3uc.part14.rar\" - 2,56 GB yEnc (71/273)",
        "[ TOWN ]-[ www.town.ag ]-[ partner of www.ssl-news.info ] [16/32] - \"6nf3wQL3uc.part14.rar\" - 2,56 GB yEnc (71/273)") == false);

    BOOST_REQUIRE(nntp::strcmp(
        "girls flirting with is neat   GiBBA files  Soft I Love you to BF-Vol3 (102).jpg (1/4)",
        "girls flirting with is neat   GiBBA files  Soft I Love you to BF-Vol3 (102).jpg (2/4)"));

    BOOST_REQUIRE(nntp::strcmp(
        "girls flirting with is neat   GiBBA files  Soft I Love you to BF-Vol3 (102).jpg (1/4)",
        "girls flirting with is neat   GiBBA files  Soft I Love you to BF-Vol3 (103).jpg (2/4)") == false);

    BOOST_REQUIRE(nntp::strcmp(
        "Metallica - Enter Sandman yEnc (01/10).mp3",
        "Metallica - Enter Sandman yEnc (02/10).mp3"));

    BOOST_REQUIRE(nntp::strcmp(
        "foobar - [02/32] - \"foobar_HD_1920x1080.mp4.001\" yEnc (3/10)",
        "foobar - [02/32] - \"foobar_HD_1920x1080.mp4.001\" yEnc (10/10)"));

   BOOST_REQUIRE(nntp::strcmp(
        "foobar - [02/32] - \"foobar_HD_1920x1080.mp4.001\" yEnc (3/10) whatever",
        "foobar - [02/32] - \"foobar_HD_1920x1080.mp4.001\" yEnc (10/10) whatever"));

   BOOST_REQUIRE(nntp::strcmp(
        "foobar - [02/32] - \"foobar_HD_1920x1080.mp4.001\" yEnc (003/100) whatever",
        "foobar - [02/32] - \"foobar_HD_1920x1080.mp4.001\" yEnc (010/100) whatever"));

   BOOST_REQUIRE(nntp::strcmp(
        "foobar - [02/32] - \"foobar_HD_1920x1080.mp4.001\" yEnc (1/100) whatever",
        "foobar - [02/32] - \"foobar_HD_1920x1080.mp4.001\" yEnc (100/100) whatever"));

   BOOST_REQUIRE(nntp::strcmp(
        "foobar - [02/32] - \"foobar_HD_1920x1080.mp4.001\" yEnc (1/100) foo",
        "foobar - [02/32] - \"foobar_HD_1920x1080.mp4.001\" yEnc (100/100) bar") == false);

}

void test_hash()
{
    BOOST_REQUIRE(nntp::hashvalue("foobar") == nntp::hashvalue("foobar"));
    BOOST_REQUIRE(nntp::hashvalue("foo") != nntp::hashvalue("bar"));
    BOOST_REQUIRE(nntp::hashvalue("[408390]-[FULL]-[#a.b.erotica@EFNet]-[ nvg.15.04.04.corrine ]-[42/55] - \"nvg.15.04.04.corrine.r32\" yEnc (62/66)") ==
        nntp::hashvalue("[408390]-[FULL]-[#a.b.erotica@EFNet]-[ nvg.15.04.04.corrine ]-[42/55] - \"nvg.15.04.04.corrine.r32\" yEnc (63/66)"));

    BOOST_REQUIRE(nntp::hashvalue("[408390]-[FULL]-[#a.b.erotica@EFNet]-[ nvg.15.04.04.corrine ]-[43/55] - \"nvg.15.04.04.corrine.r32\" yEnc (63/66)") !=
        nntp::hashvalue("[408390]-[FULL]-[#a.b.erotica@EFNet]-[ nvg.15.04.04.corrine ]-[44/55] - \"nvg.15.04.04.corrine.r32\" yEnc (63/66)"));

    BOOST_REQUIRE(nntp::hashvalue("girls flirting with is neat   GiBBA files  Soft I Love you to BF-Vol3 (102).jpg (1/1)") !=
        nntp::hashvalue("girls flirting with is neat   GiBBA files  Soft I Love you to BF-Vol3 (103).jpg (1/1)"));

    BOOST_REQUIRE(nntp::hashvalue("girls flirting with is neat   GiBBA files  Soft I Love you to BF-Vol3 (102).jpg (1/4)") ==
        nntp::hashvalue("girls flirting with is neat   GiBBA files  Soft I Love you to BF-Vol3 (102).jpg (2/4)"));

    BOOST_REQUIRE(nntp::hashvalue("heres a movie with shitty part notation foobar.avi.001 [04/50]") ==
        nntp::hashvalue("heres a movie with shitty part notation foobar.avi.001 [10/50]"));

    BOOST_REQUIRE(nntp::hashvalue("foobar - [02/32] - \"foobar_HD_1920x1080.mp4.001\" yEnc (3/10)") ==
        nntp::hashvalue("foobar - [02/32] - \"foobar_HD_1920x1080.mp4.001\" yEnc (10/10)"));

}

std::mutex mutex;
std::condition_variable cond;

bool release_thundering_herd;

void run_tests()
{
    std::unique_lock<std::mutex> lock(mutex);
    if (!release_thundering_herd)
        cond.wait(lock);
    lock.unlock();

    typedef void (*testcase)(void);

    const testcase cases[] = {
        test_reverse_iterator,
        test_parse_overview,
        test_parse_date,
        test_parse_part,
        test_parse_group,
        test_date,
        test_is_binary,
        test_find_filename,
        test_find_response,
        test_find_body,
        test_scan_response,
        test_to_int,
        test_strcmp,
        test_hash,
    };
    for (auto test : cases)
    {
        for (int i=0; i<1000; ++i)
            test();
    }
}

void test_thread_safety()
{
    std::unique_lock<std::mutex> lock(mutex);

    std::thread t0(run_tests);
    std::thread t1(run_tests);
    std::thread t2(run_tests);

    release_thundering_herd = true;
    cond.notify_all();
    lock.unlock();

    t0.join();
    t1.join();
    t2.join();
}

int test_main(int, char* [])
{
    test_reverse_iterator();
    test_parse_overview();
    test_parse_date();
    test_parse_part();
    test_parse_group();
    test_date();
    test_is_binary();
    test_find_filename();
    test_find_response();
    test_find_body();
    test_scan_response();
    test_to_int();
    test_strcmp();
    test_hash();

    test_thread_safety();

    return 0;
}


