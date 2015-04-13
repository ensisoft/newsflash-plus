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

#include <newsflash/config.h>

#include <boost/test/minimal.hpp>
#include "../index.h"
#include "../article.h"

struct storage 
{
    struct buffer {
        using iterator = char*;
        using const_iterator = const char*;
    };
};

using article = newsflash::article<storage>;

bool operator==(const article& lhs, const article& rhs)
{
    return lhs.pubdate() == rhs.pubdate() &&
           lhs.subject() == rhs.subject() &&
           lhs.author() == rhs.author() &&
           lhs.bytes() == rhs.bytes();
}

int test_main(int, char*[])
{

    article catalog[2][3];

    catalog[0][0].set_author("John dow john@gmail.com");
    catalog[0][0].set_subject("Metallica - Enter Sandman yEnc (0/3).mp3");
    catalog[0][0].set_bytes(100);
    catalog[0][0].set_pubdate(600);

    catalog[0][1].set_author("foo@gmail.com");
    catalog[0][1].set_subject("Red Dwarf Season 8.vol001+02.PAR2");
    catalog[0][1].set_bytes(150);
    catalog[0][1].set_pubdate(650);

    catalog[0][2].set_author("ano@anonymous (knetje)");
    catalog[0][2].set_subject("#A.B.MM @  EFNet Presents: REQ 40092 - Seinfeld.S09.DVDRip.XviD-SiNK - 482/520 - sink-seinfeld.s09e21e22.r23 (1/3)");
    catalog[0][2].set_bytes(50);
    catalog[0][2].set_pubdate(630);

    catalog[1][0].set_author("foo@foobar.com");
    catalog[1][0].set_subject(".net question");
    catalog[1][0].set_bytes(58);
    catalog[1][0].set_pubdate(800);

    catalog[1][1].set_author("ano@anonymous");
    catalog[1][1].set_subject("Terminator.2.Judgement.Day.h264.720p-FOO (001/100).par");
    catalog[1][1].set_bytes(1024);
    catalog[1][1].set_pubdate(500);

    // test sorting.
    {
        using index = newsflash::index<storage>;
        index i;
        i.on_load = [&](std::size_t key, std::size_t index) {
            BOOST_REQUIRE(key < 2);
            BOOST_REQUIRE(index < 3);
            return catalog[key][index];
        };

        i.sort(index::sorting::sort_by_date, index::sortdir::ascending);
        i.insert(catalog[1][0], 1, 0);
        i.insert(catalog[0][2], 0, 2);
        i.insert(catalog[0][1], 0, 1);
        i.insert(catalog[0][0], 0, 0);

        BOOST_REQUIRE(i.size() == 4);
        BOOST_REQUIRE(i[0] == catalog[0][0]);
        BOOST_REQUIRE(i[1] == catalog[0][2]);
        BOOST_REQUIRE(i[2] == catalog[0][1]);
        BOOST_REQUIRE(i[3] == catalog[1][0]);

        i.sort(index::sorting::sort_by_date, index::sortdir::descending);
        BOOST_REQUIRE(i[0] == catalog[1][0]);
        BOOST_REQUIRE(i[1] == catalog[0][1]);
        BOOST_REQUIRE(i[2] == catalog[0][2]);
        BOOST_REQUIRE(i[3] == catalog[0][0]);   

        i.insert(catalog[1][1], 1, 1);     
        BOOST_REQUIRE(i.size() == 5);
        BOOST_REQUIRE(i[0] == catalog[1][0]);
        BOOST_REQUIRE(i[1] == catalog[0][1]);
        BOOST_REQUIRE(i[2] == catalog[0][2]);
        BOOST_REQUIRE(i[3] == catalog[0][0]);
        BOOST_REQUIRE(i[4] == catalog[1][1]);
    }

    // test filtering.
    {
        using index = newsflash::index<storage>;
        index i;
        i.on_load = [&](std::size_t key, std::size_t index) {
            BOOST_REQUIRE(key < 2);
            BOOST_REQUIRE(index < 3);
            return catalog[key][index];
        };

        i.set_size_filter(1025, 1026);
        i.filter();
        i.sort(index::sorting::sort_by_date, index::sortdir::ascending);

        i.insert(catalog[0][0], 0, 0);
        i.insert(catalog[0][1], 0, 1);
        i.insert(catalog[0][2], 0, 2);        
        i.insert(catalog[1][0], 1, 0);
        i.insert(catalog[1][1], 1, 1);
        BOOST_REQUIRE(i.size() == 0);

        i.set_size_filter(1024, 1024);
        i.filter();
        BOOST_REQUIRE(i.size() == 1);

        i.set_size_filter(150, 1024);
        i.filter();
        BOOST_REQUIRE(i.size() == 2);
        BOOST_REQUIRE(i[0] == catalog[1][1]);
        BOOST_REQUIRE(i[1] == catalog[0][1]);

        i.sort(index::sorting::sort_by_size, index::sortdir::ascending);
        i.set_date_filter(1000, 1000);
        i.filter();
        BOOST_REQUIRE(i.size() == 0);
        i.set_date_filter(0, std::numeric_limits<std::time_t>::max());
        i.filter();
        BOOST_REQUIRE(i.size() == 2);
        i.set_size_filter(0, std::numeric_limits<std::uint32_t>::max());        
        i.filter();
        BOOST_REQUIRE(i.size() == 5);
        BOOST_REQUIRE(i[0] == catalog[0][2]);
        BOOST_REQUIRE(i[1] == catalog[1][0]);
        BOOST_REQUIRE(i[2] == catalog[0][0]);
        BOOST_REQUIRE(i[3] == catalog[0][1]);        
        BOOST_REQUIRE(i[4] == catalog[1][1]);                

    }

    return 0;
}