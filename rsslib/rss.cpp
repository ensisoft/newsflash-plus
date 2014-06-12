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

#include <newsflash/warnpush.h>
#  include <QRegExp>
#include <newsflash/warnpop.h>

#include <algorithm>
#include "rss.h"

namespace {
    int four_digit_year(int year)
    {
        if (year < 1000)
            return 1900 + year;
        return year;
    }

} // namespace

namespace rss
{

QDateTime parse_date(const QString& str)
{
    if (str.isEmpty())
        return QDateTime();
    
    // match a RFC 822 date for example: "Fri, 26 Feb 2010 13:47:54 +0000"
    const QRegExp regex(
        "((Mon|Tue|Wed|Thu|Fri|Sat|Sun),\\s)?(\\d{1,2}) "
        "(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec) "
        "(\\d{2,4}) "
        "(\\d{2}):(\\d{2})(:(\\d{2}))?(\\s([-+]\\d{2})\\d{2})?"
        );

    // if not matched, then not conformant data, disregard
    if (regex.indexIn(str) == -1)
        return QDateTime();    

    // process the captures.
    // cap(0) returns the input matched the whole regular expression.
    // each subsequent index returns the input that matched that part 
    // of the subexpression.
    const auto year  = four_digit_year(regex.cap(5).toInt());
    const auto day   = regex.cap(3).toInt();
    const auto hour  = regex.cap(6).toInt();
    const auto min   = regex.cap(7).toInt();
    const auto sec   = regex.cap(9).toInt();
    const auto tmz   = regex.cap(11).toInt(); // this is hours only todo: minutes
    const auto month = regex.cap(4);    

    const QString months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    const auto it = std::find(std::begin(months), std::end(months), month);
    if (it == std::end(months))
        return QDateTime();

    const auto monthno = std::distance(std::begin(months), it) + 1;

    // create a datetime disregarding timezone in the RSS
    const QDateTime datetime(
        QDate(year, monthno, day),
        QTime(hour, min, sec), 
        Qt::UTC);

    if (!datetime.isValid())
        return datetime;

    // offset by timezone seconds
    const auto off = -tmz * 3600;
    const auto ret = datetime.addSecs(off);
    return ret;
}

rss::timeframe calculate_timeframe(const QDateTime& now, const QDateTime& event)
{
    Q_ASSERT(event < now);

    const auto days = event.daysTo(now);
    if (days == 0)
        return timeframe::today;
    else if (days == 1)
        return timeframe::yesterday;
    else if (days < 6)
        return timeframe::within_a_week;

    return timeframe::older_than_week;
}

rss::timediff calculate_timediff(const QDateTime& first, const QDateTime& second)
{
    auto seconds = std::abs(first.secsTo(second));

    enum { SECONDS_TO_DAY  = 60 * 60 * 24,
           SECONDS_TO_HOUR = 60 * 60,
           SECONDS_TO_MIN  = 60
    };

    timediff diff = {};
    
    diff.days    = seconds / SECONDS_TO_DAY;
    seconds   -= diff.days * SECONDS_TO_DAY;
    diff.hours   = seconds / SECONDS_TO_HOUR;
    seconds   -= diff.hours * SECONDS_TO_HOUR;
    diff.minutes = seconds / SECONDS_TO_MIN;
    seconds   -= diff.minutes * SECONDS_TO_MIN;
    diff.seconds = seconds;    

    return diff;
}

} // rss
