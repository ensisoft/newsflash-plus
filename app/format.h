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
#include <newsflash/warnpop.h>
#include <string>
#include "types.h"

class QNetworkReply;
class QFile;
class QDir;
class QUrl;

namespace app
{
    // formatting is related to what the user sees.
    // so the data that comes out here is usable at the UI layer.


    // generic format method, supports whatever Qt QString::arg supports
    template<typename T>
    QString format(const T& t)
    {
        return QString("%1").arg(t);
    }

    QString format(const void* ptr);
    QString format(bool val);
    QString format(const QFile& file);
    QString format(const QDir& dir);
    QString format(const QUrl& url);
    QString format(const QNetworkReply& reply);
    QString format(const QStringList& list);
    QString format(const app::size& size);
    QString format(const app::speed& speed);
    QString format(const app::gigs& gigs);
    QString format(const app::megs& megs);
    QString format(const app::event& event);
    QString format(const app::age& age);
    QString format(const app::runtime& rt);
    QString format(const app::etatime& eta);
    QString format(const app::volume& vol);    
    QString format(const std::string& str);        


    namespace detail {
        inline
        QString key(int index)
        {
            return QString("_%1").arg(index);
        }

        template<typename T>
        void format_arg(QString& s, int index, const T& t)
        {
            const auto& str = app::format(t);
            s = s.replace(key(index), str);
        }

        template<typename T, typename... Rest>
        void format_arg(QString& s, int index, const T& t, const Rest&... rest)
        {
            const auto& str = app::format(t);
            s = s.replace(key(index), str);
            format_arg(s, index + 1, rest...);
        }
    } // detail

    // a wrapper around QString::arg() with more convenient syntax
    // and overloads for supporting more types for formatting.
    template<typename... Args>
    QString str(const char* fmt, const Args&... args)
    {
        QString s(fmt);
        detail::format_arg(s, 1, args...);
        return s;
    }

    template<typename T>
    QString str(const T& arg)
    {
        return str("_1", arg);
    }

    inline
    QString str(const char* str) 
    {
        return QString { str };
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
    std::string toUtf8(const QString& str)
    {
        const auto& bytes = str.toUtf8();
        return std::string(bytes.data(), bytes.size());
    }

    inline
    std::string toLatin(const QString& str)
    {
        const auto& bytes = str.toLatin1();
        return std::string(bytes.data(), bytes.size());
    }

    inline
    QString fromUtf8(const std::string& s)
    {
        return QString::fromUtf8(s.c_str());
    }

    inline 
    QString fromLatin(const std::string& s)
    {
        return QString::fromLatin1(s.c_str());
    }


    // get a string in systems native narrow character encoding.
    std::string narrow(const QString& str);

    QString widen(const std::string& s);

    // get a Qstring from a native system string.
    // the string is expected to be in the systems 
    // native narrow character encoding (whatever that is)
    //QString widen(const char* str);
    //QString widen(const wchar_t* str);



} // sdk

