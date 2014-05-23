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

#pragma once

#include <newsflash/warnpush.h>
#  include <QString>
#  include <QDateTime>
#include <newsflash/warnpop.h>

namespace rss
{
    enum class timeframe {
        today,
        yesterday,
        within_a_week,
        older_than_week
    };

    struct timediff {
        int days;
        int hours;
        int minutes;
        int seconds;
    };

    // Try to parse a date from a RSS feed into a valid date object.
    // The expected date format is "Fri, 26 Feb 2010 13:47:54 +0000",
    // i.e. the date format specified in RFC 822/2822.
    // The returned QDateTime is in UTC time zone.
    // If string is not a valid date returns invalid QDateTime    
    QDateTime parse_date(const QString& str);

    // Calculate and simply the the difference between two events.
    // returns a value that describes when the an event occurred with respect to
    // current time (given in now). 
    // for Example if now represents 25th of April 2014 13:00 and event is
    // 24th April 2014 14:00 then timeframe == yesterday
    // event is required to be less than now.
    timeframe calculate_timeframe(const QDateTime& now, const QDateTime& event);

    // Breakdown difference between two timestamps
    timediff calculate_timediff(const QDateTime& first, const QDateTime& second);

} // rss
