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
#  include <QFile>
#  include <QDir>
#include <newsflash/warnpop.h>

#if defined(WINDOWS_OS)
#  include <windows.h>
#elif defined(LINUX_OS)
#  include <langinfo.h> // for nl_langinfo
#endif

#include <cstring>
#include "format.h"

namespace {

    // use floating points here so we get automatic
    // promotion, and no need to cast in format funcs
    const double GB = 1024 * 1024 * 1024;
    const double MB = 1024 * 1024;
    const double KB = 1024;

} // namespace

namespace sdk
{


namespace detail {
    void format(QString& s, int index, const sdk::size& size)
    {
        QString str;
        if (size.bytes >= GB)
            str = QString("%1 Gb").arg(size.bytes / GB, 0, 'f', 1, ' ');
        else if (size.bytes >= MB)
            str = QString("%1 Mb").arg(size.bytes / MB, 0, 'f', 1, ' ');
        else str = QString("%1 Kb").arg(size.bytes / KB, 0, 'f', 1, ' ');

        s = s.replace(key(index), str);
    }

    void format(QString& s, int index, const sdk::speed& speed)
    {
        QString str;
        if (speed.bps >= GB)
            str = QString("%1 Gb/s").arg(speed.bps / GB, 0, 'f', 1);
        else if (speed.bps >= MB)
            str = QString("%1 Mb/s").arg(speed.bps / MB, 0, 'f', 1);
        else str = QString("%1 Kb/s").arg(speed.bps / KB, 0, 'f', 1);

        s = s.replace(key(index), str);
    }

    void format(QString& s, int index, const QFile& file)
    {
        s = s.replace(key(index), QString("'%1'").arg(file.fileName()));
    }

    void format(QString& s, int index, const QDir& dir)
    {
        const QString& path = QDir::toNativeSeparators(dir.absolutePath());
        s = s.replace(key(index), QString("'%1'").arg(path));
    }    
}


std::string narrow(const QString& str)
{
#if defined(WINDOWS_OS)
    return utf8(str);
#elif defined(LINUX_OS)
    const char* codeset = nl_langinfo(CODESET);
    if (!std::strcmp(codeset, "UTF-8"))
        return utf8(str);

    const auto& bytes = str.toLocal8Bit();
    return std::string(bytes.data(),
        bytes.size());
#endif
}

QString widen(const char* str)
{
#if defined(WINDOWS_OS)
    return QString::fromLatin1(str);

#elif defined(LINUX_OS)
    const char* codeset = nl_langinfo(CODESET);
    if (!std::strcmp(codeset, "UTF-8"))
        return QString::fromUtf8(str);
    return QString::fromLocal8Bit(str);
#endif
}

QString widen(const wchar_t* str)
{
    // windows uses UTF-16 (UCS-2 with surrogate pairs for non BMP characters)    
    // if this gives a compiler error it means that wchar_t is treated
    // as a native type by msvc. In order to compile wchar_t needs to be
    // unsigned short. Doing a cast will help but then a linker error will 
    // follow since Qt build assumes that wchar_t = unsigned short
#if defined(WINDOWS_OS)
    return QString::fromUtf16(str);
#elif defined(LINUX_OS)
    static_assert(sizeof(wchar_t) == sizeof(uint), "");

    return QString::fromUcs4((const uint*)str);
#endif
}

double gigs(quint64 bytes)
{
    return bytes / GB;
}

} // sdk