// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
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

#include <boost/test/minimal.hpp>

#include "../rss.h"

void unit_test_parse()
{
    // The expected date format is "Fri, 26 Feb 2010 13:47:54 +0000",

    // incorrect
    {
        auto dt = rss::parse_date("fooboasgas");
        BOOST_REQUIRE(!dt.isValid());

        dt = rss::parse_date("Fri, 34 Feb 2013 12:00:00 +0000");
        BOOST_REQUIRE(!dt.isValid());

        dt = rss::parse_date("Fri, 12 Feb 201345 12:00:00 +0000");
        BOOST_REQUIRE(!dt.isValid());
    }

    // correct
    {
        auto dt = rss::parse_date("Fri, 26 Feb 2010 13:47:54 +0000");
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

void unit_test_timeframe()
{
    const auto& now = QDateTime::currentDateTime();

    BOOST_REQUIRE(rss::calculate_timeframe(now, now) == rss::timeframe::today);
    BOOST_REQUIRE(rss::calculate_timeframe(now, now.addDays(-1)) == rss::timeframe::yesterday);
    BOOST_REQUIRE(rss::calculate_timeframe(now, now.addDays(-5)) == rss::timeframe::within_a_week);
}

void unit_test_timediff()
{
    
}

int test_main(int argc, char* argv[])
{
    unit_test_parse();
    unit_test_timeframe();

    return 0;
}