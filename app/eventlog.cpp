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
#  include <QtGui/QIcon>
#include <newsflash/warnpop.h>

#define LOGTAG

#include "eventlog.h"
#include "debug.h"

namespace app
{

eventlog::eventlog() : events_(200)
{}

eventlog::~eventlog()
{}

void eventlog::write(event type, const QString& msg, const QString& tag)
{
    const auto time = QTime::currentTime();
    const event_t e {type, msg, tag, time };

    if (events_.full())
    {
        const auto first = index(0, 0);
        const auto last  = index((int)events_.size(), 1);
        events_.push_front(e);
        emit dataChanged(first, last);
    }
    else
    {
        beginInsertRows(QModelIndex(), 0, 0);
        events_.push_front(e);
        endInsertRows();
    }    
}

void eventlog::clear()
{
    beginRemoveRows(QModelIndex(), 0, events_.size());
    events_.clear();
    endRemoveRows();
}

QAbstractItemModel* eventlog::view()
{
    return this;
}

QString eventlog::name() const
{
    return "eventlog";
}

int eventlog::rowCount(const QModelIndex&) const
{
    return static_cast<int>(events_.size());
}

QVariant eventlog::data(const QModelIndex& index, int role) const
{
    const auto& ev = events_[index.row()];
    switch (role)
    {
        case Qt::DecorationRole:
        switch (ev.type) {
            case eventlog::event::warning:
                return QIcon(":/resource/16x16_ico_png/ico_warning.png");
            case eventlog::event::info:
                return QIcon(":/resource/16x16_ico_png/ico_info.png");
            case eventlog::event::error:
                return QIcon(":/resource/16x16_ico_png/ico_error.png");
            default:
                Q_ASSERT("missing event type");
                break;
            }
            break;

            case Qt::DisplayRole:
                return QString("[%1] [%2] %3")
                    .arg(ev.time.toString("hh:mm:ss:zzz"))
                    .arg(ev.logtag)
                    .arg(ev.message);                
                    break;
    }
    return QVariant();    
}

eventlog& eventlog::get()
{
    static eventlog log;
    return log;
}

} // app
    