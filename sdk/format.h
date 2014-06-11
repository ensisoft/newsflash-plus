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
#  include <QtGlobal>
#  include <QString>
#include <newsflash/warnpop.h>
#include <string>


class QFile;
class QDir;

namespace sdk
{
    struct size {
        quint64 bytes;
    };

    struct speed {
        quint32 bps;
    };

    struct gigs {
        gigs() : bytes(0)
        {}

        gigs(quint64 bytes) : bytes(bytes)
        {}

        gigs(double gb) : bytes(gb * 1024 * 1024 * 1024)
        {}

        double as_float() const {
            static auto d = 1024.0 * 1024.0 * 1024.0;
            return bytes / d;
        }

        quint64 as_bytes() const {
            return bytes;
        }
    private:
        quint64 bytes;
    };

    struct megs {
        megs() : bytes(0)
        {}

        megs(quint64 bytes) : bytes(bytes)
        {}

        megs(double mb) : bytes(mb * 1024 * 1024)
        {}

        double as_float() const {
            static auto d = 1024.0 * 1024.0;
            return bytes / d;
        };
        quint64 as_bytes() const {
            return bytes;
        }
    private:
        quint64 bytes;
    };



    namespace detail {
        inline
        QString key(int index)
        {
            return QString("_%1").arg(index);
        }


        template<typename T>
        void format(QString& s, int index, const T& t)
        {
            s = s.replace(key(index), QString("%1").arg(t));
        }

        inline
        void format(QString& s, int index, bool val)
        {
            s = s.replace(key(index), (val ? "True" : "False"));
        }
        inline
        void format(QString& s, int index, std::string str)
        {
            s = s.replace(key(index), str.c_str());
        }

        void format(QString& s, int index, const QFile& file);
        void format(QString& s, int index, const QDir& dir);
        void format(QString& s, int index, const sdk::size& size);
        void format(QString& s, int index, const sdk::speed& speed);
        void format(QString& s, int index, const sdk::gigs& gigs);
        void format(QString& s, int index, const sdk::megs& megs);

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

    template<typename T>
    QString str(const T& arg)
    {
        return str("_1", arg);
    }

    template<typename... Args>
    std::string str_a(const char* fmt, const Args&... args)
    {
        const auto& string = str(fmt, args...);
        const auto& bytes  = string.toAscii();
        return std::string(
            bytes.data(), bytes.size());
    }

    inline
    std::string utf8(const QString& str)
    {
        const auto& bytes = str.toUtf8();
        return std::string(bytes.data(), 
            bytes.size());
    }

    inline
    std::string latin(const QString& str)
    {
        const auto& bytes = str.toLatin1();
        return std::string(bytes.data(),
            bytes.size());
    }

    // get a string in systems native narrow character encoding.
    std::string narrow(const QString& str);

    // get a Qstring from a native system string.
    // the string is expected to be in the systems 
    // native narrow character encoding (whatever that is)
    QString widen(const char* str);

    QString widen(const wchar_t* str);


} // sdk

