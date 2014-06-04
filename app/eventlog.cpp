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
#  include <QCoreApplication>
#include <newsflash/warnpop.h>

#include "eventlog.h"
#include "foobar.h"

namespace app
{

eventlog::eventlog() : events_(200)
{}

eventlog::~eventlog()
{}

void eventlog::hook(QCoreApplication& app)
{
    // enable this eventlog as the "global" event log so that
    // it works with the logging macros
    app.installEventFilter(this);
}

void eventlog::unhook(QCoreApplication& app)
{
    app.removeEventFilter(this);
}

void eventlog::write(event_t type, const QString& msg, const QString& ctx)
{
    insert(type, msg, ctx, QTime::currentTime());
}

void eventlog::clear()
{
    beginRemoveRows(QModelIndex(), 0, events_.size());
    events_.clear();
    endRemoveRows();
}


int eventlog::rowCount(const QModelIndex&) const
{
    return static_cast<int>(events_.size());
}

QVariant eventlog::data(const QModelIndex& index, int role) const
{
    const auto& event = events_[index.row()];
    switch (role)
    {
        case Qt::DecorationRole:
        switch (event.type) {
            case app::eventlog::event_t::warning:
                return QIcon(":/resource/16x16_ico_png/ico_warning.png");
            case app::eventlog::event_t::info:
                return QIcon(":/resource/16x16_ico_png/ico_info.png");
            case app::eventlog::event_t::error:
                return QIcon(":/resource/16x16_ico_png/ico_error.png");
            default:
                Q_ASSERT("missing event type");
                break;
            }
            break;

            case Qt::DisplayRole:
                return QString("[%1] [%2] %3")
                    .arg(event.time.toString("hh:mm:ss:zzz"))
                    .arg(event.context)
                    .arg(event.message);                
                    break;
    }
    return QVariant();    
}

bool eventlog::eventFilter(QObject* object, QEvent* event)
{
    if (event->type() != logevent::type())
        return false;

    const auto* data = static_cast<const logevent*>(event);

    insert(data->type_, data->message_, data->context_, data->time_);
    return true;
}

void eventlog::insert(eventlog::event_t type, const QString& msg, const QString& ctx, const QTime& time)
{
    const event e {type, msg, ctx, time};

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

void eventlog::post(event_t type, const QString& msg, const QString& ctx)
{
    static foobar foo;

    QCoreApplication::postEvent(&foo, new logevent(type, msg, ctx));
}

} // app
