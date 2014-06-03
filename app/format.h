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

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QString>
#  include <QFile>
#  include <QDir>
#include <newsflash/warnpop.h>

#include <string>

namespace app
{
    namespace detail {

        template<typename T>
        void format(QString& s, int index, const T& t)
        {
            s = s.replace(
                QString("_%1").arg(index), 
                QString("%1").arg(t));
        }

        inline
        void format(QString& s, int index, const QFile& file)
        {
            s = s.replace(
                QString("_%1").arg(index),
                QString("'%1'").arg(file.fileName()));
        }

        inline
        void format(QString& s, int index, const QDir& dir)
        {
            const QString& path = QDir::toNativeSeparators(dir.absolutePath());
            s = s.replace(
                QString("_%1").arg(index),
                QString("'%1'").arg(path));
        }

        inline
        void format(QString& s, int index, bool val)
        {
            const auto& key = QString("_%1").arg(index);
            s = s.replace(key, (val ? "True" : "False"));
        }

        template<typename T>
        void format_next(QString& s, int index, const T& t)
        {
            format(s, index, t);
        }

        template<typename T, typename... Rest>
        void format_next(QString& s, int index, const T& t, const Rest&... rest)
        {
            format(s, index, t);
            format_next(s, index + 1, rest...);
        }
    }

    // a wrapper around QString::arg() with more convenient syntax
    // and also potentail for changing the output by overloading format_next
    template<typename... Args>
    QString str(const char* fmt, const Args&... args)
    {
        QString s(fmt);
        detail::format_next(s, 1, args...);
        return s;
    }

    template<typename... Args>
    std::string str_a(const char* fmt, const Args&... args)
    {
        const auto& string = str(fmt, args...);
        const auto& bytes  = string.toAscii();
        return std::string(
            bytes.data(), bytes.size());
    }

    QString err(int syserr);

    inline
    std::string utf8(const QString& str)
    {
        const auto& bytes = str.toUtf8();
        return std::string(bytes.data(), bytes.size());
    }

} // 