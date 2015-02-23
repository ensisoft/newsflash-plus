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

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGlobal>
#  include <QStringList>
#  include <QString>
#  include <QDateTime>
#include <newsflash/warnpop.h>

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

    struct count {
        count() : numItems(0)
        {}
        count(quint64 numItems) : numItems(numItems)
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

} // app
