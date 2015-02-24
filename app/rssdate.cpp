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

#include <newsflash/warnpush.h>
#  include <QRegExp>
#include <newsflash/warnpop.h>
#include <algorithm>
#include "rssdate.h"

namespace {
    int fourDigitYear(int year)
    {
        if (year < 1000)
            return 1900 + year;
        return year;
    }

} // namespace

namespace app
{

QDateTime parseRssDate(const QString& str)
{
    // match a RFC 822 date for example: "Fri, 26 Feb 2010 13:47:54 +0000"
    const QRegExp rfc822(
        "((Mon|Tue|Wed|Thu|Fri|Sat|Sun),\\s)?(\\d{1,2}) "
        "(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec) "
        "(\\d{2,4}) "
        "(\\d{2}):(\\d{2})(:(\\d{2}))?(\\s([-+]\\d{2})\\d{2})?"
        );

    if (rfc822.indexIn(str) != -1)
    {
        // process the captures.
        // cap(0) returns the input matched the whole regular expression.
        // each subsequent index returns the input that matched that part 
        // of the subexpression.
        const auto year  = fourDigitYear(rfc822.cap(5).toInt());
        const auto day   = rfc822.cap(3).toInt();
        const auto hour  = rfc822.cap(6).toInt();
        const auto min   = rfc822.cap(7).toInt();
        const auto sec   = rfc822.cap(9).toInt();
        const auto tmz   = rfc822.cap(11).toInt(); // this is hours only todo: minutes
        const auto month = rfc822.cap(4);    

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
            return QDateTime();

        // offset by timezone seconds
        const auto off = -tmz * 3600;
        const auto ret = datetime.addSecs(off);
        return ret;
    }

    return QDateTime();
}

} // app
