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

#pragma once

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QtNetwork/QNetworkReply>
#  include <QProcess>
#  include <QString>
#  include <QFile>
#  include <QUrl>
#include "newsflash/warnpop.h"

#include <string>

#include "engine/ui/task.h"

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

    QString widen(const std::string& s);

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

    inline QString toString(const std::string& s)
    { return widen(s); }

    inline QString toString(const char* str)
    { return str; }

    inline QString toString(const QString& str)
    { return str; }

    inline QString toString(const QUrl& url)
    { return url.toString(); }

    inline QString toString(bool value)
    { return (value ? "True" : "False"); }

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

