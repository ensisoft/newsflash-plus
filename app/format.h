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
#  include <QtNetwork/QNetworkReply>
#  include <QProcess>
#  include <QString>
#  include <QFile>
#  include <QUrl>
#include <newsflash/warnpop.h>
#include <newsflash/engine/ui/task.h>
#include <string>
#include "filetype.h"
#include "media.h"

namespace app
{
    // formatting is related to what the user sees.
    // so the data that comes out here is usable at the UI layer.
    // if localization is to be done this is the place where
    // object information is to be localized. The formatting strings
    // need to be done elsewhere though. (currently dont exist)

    struct size;
    struct speed;
    struct runtime;
    struct etatime;
    struct count;
    struct gigs;
    struct megs;
    struct event;
    struct age;

    QString toString(Media media);
    QString toString(filetype type);

    QString toString(QFile::FileError error);
    QString toString(QNetworkReply::NetworkError error);
    QString toString(QProcess::ProcessError error);

    QString toString(const app::size& size);
    QString toString(const app::speed& speed);
    QString toString(const app::gigs& gigs);
    QString toString(const app::megs& megs);
    QString toString(const app::event& event);
    QString toString(const app::age& age);
    QString toString(const app::runtime& rt);
    QString toString(const app::etatime& eta);
    QString toString(const app::count& vol);    

    inline QString toString(const char* str)
    { return str; }

    inline QString toString(const QString& str)
    { return str; }

    inline QString toString(const QUrl& url)
    { return url.toString(); }

    // generic format method, supports whatever Qt QString::arg supports
    template<typename T>
    QString toString(const T& value)
    {
        return QString("%1").arg(value);
    }
    template<typename T, typename... Args>
    QString toString(QString fmt, const T& value, const Args&... args)
    {
        fmt = fmt.arg(toString(value));
        return toString(fmt, args...);
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

} // app

