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
#  include <QtNetwork/QNetworkReply>
#  include <QFile>
#  include <QDir>
#  include <QUrl>
#include <newsflash/warnpop.h>

#if defined(WINDOWS_OS)
#  include <windows.h>
#elif defined(LINUX_OS)
#  include <langinfo.h> // for nl_langinfo
#endif

#include <sstream>
#include <cstring>
#include "format.h"

namespace {

    // use floating points here so we get automatic
    // promotion, and no need to cast in format funcs
    const double GB = 1024 * 1024 * 1024;
    const double MB = 1024 * 1024;
    const double KB = 1024;

} // namespace

namespace app
{

QString format(const app::volume& vol)
{
    const auto BILLION  = 1000 * 1000 * 1000.0;
    const auto MILLION  = 1000 * 1000.0;
    const auto THOUSAND = 1000.0;

    if (vol.numItems > BILLION)
        return QString("%1 b").arg(vol.numItems / BILLION, 0, 'f', 1, ' ');
    if (vol.numItems > MILLION)
        return QString("%1 m").arg(vol.numItems / MILLION, 0, 'f', 1, ' ');
    if (vol.numItems > THOUSAND)
        return QString("%1 k").arg(vol.numItems / THOUSAND, 0, 'f', 1, ' ');

    return QString("%1").arg(vol.numItems);

}

QString format(const app::size& size)
{
    if (size.bytes >= GB)
        return QString("%1 Gb").arg(size.bytes / GB, 0, 'f', 1, ' ');
    else if (size.bytes >= MB)
        return QString("%1 Mb").arg(size.bytes / MB, 0, 'f', 1, ' ');

    return QString("%1 Kb").arg(size.bytes / KB, 0, 'f', 1, ' ');
}

QString format(const app::speed& speed)
{
    if (speed.bps >= GB)
        return QString("%1 Gb/s").arg(speed.bps / GB, 0, 'f', 1);
    else if (speed.bps >= MB)
        return QString("%1 Mb/s").arg(speed.bps / MB, 0, 'f', 1);

    return QString("%1 Kb/s").arg(speed.bps / KB, 0, 'f', 1);
}

QString format(const app::gigs& gigs)
{
    return QString("%1 Gb").arg(gigs.as_float(), 0, 'f', 1, ' ');
}

QString format(const app::megs& megs)
{
    return QString("%1 Mb").arg(megs.as_float(), 0, 'f', 1, ' ');
}

QString format(const app::event& event)
{
    if (!event.valid())
        return "???";

    // These expressions may be used for the date:
    // Expression          Output
    // d                   the day as number without a leading zero (1 to 31)
    // dd                  the day as number with a leading zero (01 to 31)
    // ddd                 the abbreviated localized day name (e.g. 'Mon' to 'Sun'). Uses QDate::shortDayName().
    // dddd                the long localized day name (e.g. 'Monday' to 'Qt::Sunday'). Uses QDate::longDayName().
    // M                   the month as number without a leading zero (1-12)
    // MM                  the month as number with a leading zero (01-12)
    // MMM                 the abbreviated localized month name (e.g. 'Jan' to 'Dec'). Uses QDate::shortMonthName().
    // MMMM                the long localized month name (e.g. 'January' to 'December'). Uses QDate::longMonthName().
    // yy                  the year as two digit number (00-99)
    // yyyy                the year as four digit number

    // These expressions may be used for the time:
    // Expression          Output
    // h                   the hour without a leading zero (0 to 23 or 1 to 12 if AM/PM display)
    // hh                  the hour with a leading zero (00 to 23 or 01 to 12 if AM/PM display)
    // m                   the minute without a leading zero (0 to 59)
    // mm                  the minute with a leading zero (00 to 59)
    // s                   the second without a leading zero (0 to 59)
    // ss                  the second with a leading zero (00 to 59)
    // z                   the milliseconds without leading zeroes (0 to 999)
    // zzz                 the milliseconds with leading zeroes (000 to 999)
    // AP                  use AM/PM display. AP will be replaced by either "AM" or "PM".
    // ap                  use am/pm display. ap will be replaced by either "am" or "pm".

    QString str;

    // todo: fix this. dddd is *localized* but we have hardcoded english here 


    const auto date = event.datetime();
    const auto when = event.as_when();
    switch (when)
    {
        case app::event::when::today:
        str = date.toString("'Today' hh:mm");
        break;

        case app::event::when::yesterday:
        str = date.toString("'Yesterday' hh:mm");
        break;

        case app::event::when::this_week:
        str = date.toString("dddd hh:mm");
        break;

        case app::event::when::this_month:
        str = date.toString("dddd dd MMM hh:mm");
        break;

        case app::event::when::this_year:
        str = date.toString("dddd dd MMM hh:mm");
        break;

        case app::event::when::before:
        str = date.toString("dddd dd MMM hh:mm");
        break;
    }
    return str;
}

QString format(const app::age& age)
{
    return QString("%1 days").arg(age.days());
}

QString format(const app::runtime& rt)
{
    const auto hours = rt.value / 3600;
    const auto mins  = (rt.value - hours * 3600) / 60;
    const auto secs  = (rt.value - hours * 3600 - mins * 60);

    if (hours)
        return QString("%1:%2:%3")
          .arg(hours, 2, 10, QChar('0'))
          .arg(mins, 2, 10, QChar('0'))
          .arg(secs, 2, 10, QChar('0'));

    return QString("%1:%2")
        .arg(mins, 2, 10, QChar('0'))
        .arg(secs, 2, 10, QChar('0'));
}

QString format(const app::etatime& eta)
{
    const auto MINUTE = 60;
    const auto HOUR = 60 * 60;

    if (eta.value < MINUTE)
        return QString("%1 sec").arg(eta.value);
    if (eta.value < HOUR)
        return QString("%1 min").arg(eta.value / MINUTE);

    const auto hour = eta.value / HOUR;
    const auto min  = eta.value % MINUTE;
    return QString("%1 hour %2 min").arg(hour).arg(min);
}

QString format(const std::string& str)
{
    return QString(str.c_str());
}

QString format(const void* ptr)
{
    std::stringstream ss;
    ss << ptr; 
    return QString::fromStdString(ss.str());
}

QString format(bool val)
{
    return QString((val ? "True" : "False"));
}

QString format(const QNetworkReply& reply)
{
    return reply.errorString();
}

QString format(const QFile& file)
{
    return QString("'%1'").arg(file.fileName());
}

QString format(const QDir& dir)
{
    return QString("'%1'").arg(
        QDir::toNativeSeparators(dir.absolutePath()));
}    

QString format(const QUrl& url)
{
    return QString("'%1'").arg(url.toString());
}

QString format(const QStringList& list)
{
    return list.join(", ");
}

std::string narrow(const QString& str)
{
#if defined(WINDOWS_OS)
    return to_utf8(str);
#elif defined(LINUX_OS)
    const char* codeset = nl_langinfo(CODESET);
    if (!std::strcmp(codeset, "UTF-8"))
        return toUtf8(str);

    const auto& bytes = str.toLocal8Bit();
    return std::string(bytes.data(),
        bytes.size());
#endif
}

QString widen(const std::string& s)
{ 
#if defined(WINDOWS_OS)
    return QString::fromUtf8(s.c_str());

#elif defined(LINUX_OS)
    const char* codeset = nl_langinfo(CODESET);
    if (!std::strcmp(codeset, "UTF-8"))
        return fromUtf8(s);

    return QString::fromLocal8Bit(s.c_str());
#endif
}

// QString widen(const wchar_t* str)
// {
//     // windows uses UTF-16 (UCS-2 with surrogate pairs for non BMP characters)    
//     // if this gives a compiler error it means that wchar_t is treated
//     // as a native type by msvc. In order to compile wchar_t needs to be
//     // unsigned short. Doing a cast will help but then a linker error will 
//     // follow since Qt build assumes that wchar_t = unsigned short
// #if defined(WINDOWS_OS)
//     return QString::fromUtf16(str);
// #elif defined(LINUX_OS)
//     static_assert(sizeof(wchar_t) == sizeof(uint), "");

//     return QString::fromUcs4((const uint*)str);
// #endif
// }

} // app
