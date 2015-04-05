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

using namespace newsflash;

int test_main(int, char*[])
{
    article catalog[2][3];

    catalog[0][0].author  = "John dow john@gmail.com";
    catalog[0][0].subject = "Metallica - Enter Sandman yEnc (0/3).mp3";
    catalog[0][0].bytes   = 100;
    catalog[0][0].pubdate = 600;
    catalog[0][0].offset  = 0;

    catalog[0][1].author  = "foo@gmail.com";
    catalog[0][1].subject = "Red Dwarf Season 8.vol001+02.PAR2";
    catalog[0][1].bytes   = 150;
    catalog[0][1].pubdate = 650;
    catalog[0][1].offset  = 1;

    catalog[0][2].author  = "ano@anonymous (knetje)";
    catalog[0][2].subject = "#A.B.MM @  EFNet Presents: REQ 40092 - Seinfeld.S09.DVDRip.XviD-SiNK - 482/520 - sink-seinfeld.s09e21e22.r23 (1/3)";
    catalog[0][2].bytes   = 50;
    catalog[0][2].pubdate = 630;
    catalog[0][2].offset  = 2;

    catalog[1][0].author  = "foo@foobar.com";
    catalog[1][0].subject = ".net question";
    catalog[1][0].bytes   = 58;
    catalog[1][0].pubdate = 800;
    catalog[1][0].offset  = 3;

    catalog[1][1].author  = "ano@anonymous";
    catalog[1][1].subject = "Terminator.2.Judgement.Day.h264.720p-FOO (001/100).par";
    catalog[1][1].bytes   = 1024 * 1024;
    catalog[1][1].pubdate = 1;
    catalog[1][1].offset  = 4;


    {

        newsflash::index i;
        i.on_load = [&](std::size_t key, std::size_t index) {
            BOOST_REQUIRE(key < 2);
            BOOST_REQUIRE(index < 3);
            return catalog[key][index];
        };

        i.sort(index::sorting::sort_by_age, index::order::ascending);
        i.insert(catalog[1][0], 1, 0);
        i.insert(catalog[0][2], 0, 2);
        i.insert(catalog[0][1], 0, 1);
        i.insert(catalog[0][0], 0, 0);

        BOOST_REQUIRE(i.size() == 4);
        BOOST_REQUIRE(i[0] == catalog[0][0]);
        BOOST_REQUIRE(i[1] == catalog[0][2]);
        BOOST_REQUIRE(i[2] == catalog[0][1]);
        BOOST_REQUIRE(i[3] == catalog[1][0]);

        i.sort(index::sorting::sort_by_age, index::order::descending);
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


    return 0;
}