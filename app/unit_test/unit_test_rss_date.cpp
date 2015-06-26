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
#include "../rssdate.h"

void unit_test_parse()
{
    // The expected date format is "Fri, 26 Feb 2010 13:47:54 +0000",

    // incorrect
    {
        auto dt = app::parseRssDate("fooboasgas");
        BOOST_REQUIRE(!dt.isValid());

        dt = app::parseRssDate("Fri, 34 Feb 2013 12:00:00 +0000");
        BOOST_REQUIRE(!dt.isValid());

        dt = app::parseRssDate("Fri, 12 Feb 201345 12:00:00 +0000");
        BOOST_REQUIRE(!dt.isValid());
    }

    // correct
    {
        auto dt = app::parseRssDate("Fri, 26 Feb 2010 13:47:54 +0000");
        BOOST_REQUIRE(dt.isValid());

        auto date = dt.date();
        BOOST_REQUIRE(date.day() == 26);
        BOOST_REQUIRE(date.month() == 2);
        BOOST_REQUIRE(date.year() == 2010);

        auto time = dt.time();
        BOOST_REQUIRE(time.hour() == 13);
        BOOST_REQUIRE(time.minute() == 47);
        BOOST_REQUIRE(time.second() == 54);
    }
}

int test_main(int argc, char* argv[])
{
    unit_test_parse();

    return 0;
}