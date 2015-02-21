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
#  include <QStringList>
#  include <QString>
#  include <QDateTime>
#  include <QUrl>
#include <newsflash/warnpop.h>
#include <string>

class QLibrary;
class QFile;
class QDir;

namespace app
{
    struct size {
        quint64 bytes;
    };

    struct speed {
        quint32 bps;
    };

    struct runtime {
        quint32 value;
    };    

    struct etatime {
        quint32 value;
    };

    struct volume {
        volume() : numItems(0)
        {}
        volume(quint64 numItems) : numItems(numItems)
        {}
        quint64 numItems;
    };


    struct gigs {
        gigs() : bytes(0)
        {}

        gigs(quint64 bytes) : bytes(bytes)
        {}

        gigs(double gb) : bytes(gb * 1024 * 1024 * 1024)
        {}

        double as_float() const 
        {
            static auto d = 1024.0 * 1024.0 * 1024.0;
            return bytes / d;
        }

        quint64 as_bytes() const 
        {
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

        double as_float() const 
        {
            static auto d = 1024.0 * 1024.0;
            return bytes / d;
        };
        quint64 as_bytes() const 
        {
            return bytes;
        }
    private:
        quint64 bytes;
    };

    struct event {
        event() : event_(QDateTime::currentDateTime())
        {}

        event(QDateTime event) : event_(event)
        {}

        enum class when {
            today, yesterday, this_week, this_month, this_year, before
        };

        bool valid() const
        { return event_.isValid(); }

        when as_when() const 
        {
            const auto now  = QDateTime::currentDateTime();
            const auto days = event_.daysTo(now);

            if (days == 0)
                return when::today;
            else if (days == 1)
                return when::yesterday;
            else if (event_.date().year() == now.date().year())
            {
                if (event_.date().weekNumber() == now.date().weekNumber())
                    return when::this_week;
                if (event_.date().month() == now.date().month())
                    return when::this_month;
                
                return when::this_year;
            }
            return when::before;
        }
        const QDateTime& datetime() const {
            return event_;
        }
    private:
        QDateTime event_;
    };

    struct age {
        age() : birth_(QDateTime::currentDateTime())
        {
            now_ = QDateTime::currentDateTime();
        }

        age(QDateTime birthday) : birth_(std::move(birthday))
        {
            now_ = QDateTime::currentDateTime();
        }

        quint32 days() const 
        {
            return birth_.daysTo(now_);
        }
    private:
        QDateTime birth_;
        QDateTime now_;
    };




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
    QString format(const QLibrary& lib);
    QString format(const QUrl& url);
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

