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

#include <QCoreApplication>
#include <QDebug>
#include "eventlog.h"
#include "foobar.h"

namespace app
{

eventlog::eventlog() : events_(200), app_(nullptr)
{}

eventlog::~eventlog()
{
    if (app_)
        app_->removeEventFilter(this);
}

void eventlog::make_global(QCoreApplication* app)
{
    Q_ASSERT(!app_);

    // enable this eventlog as the "global" event log so that
    // it works with the logging macros
    app->installEventFilter(this);
    app_ = app;
}

void eventlog::write(event_t type, const QString& msg, const QString& ctx)
{
    const auto time = QTime::currentTime();

    if (type == event_t::debug)
    {
        qDebug() << time << ctx << msg;
        return;
    }

    bool was_full = events_.full();
    events_.push_front({ type, msg, ctx, time});

    if (was_full)
         emit update_event(0);
    else emit insert_event(0);
}

void eventlog::clear()
{
    events_.clear();
    emit clear_events();
}

const eventlog::event& eventlog::operator[](int i) const
{
    Q_ASSERT(i < events_.size());
    return events_[i];
}

std::size_t eventlog::size() const
{
    return events_.size();
}

bool eventlog::eventFilter(QObject* object, QEvent* event)
{
    if (event->type() != logevent::type())
        return false;

    const auto* data = static_cast<const logevent*>(event);

    if (data->type_ == event_t::debug)
    {
        qDebug() << data->time_ << data->context_ << data->message_;
        return true;
    }

    bool was_full = events_.full();
    events_.push_front({ data->type_, data->message_, data->context_, data->time_ });
    if (was_full)
         emit update_event(0);
    else emit insert_event(0);
    return true;
}

void eventlog::post(event_t type, const QString& msg, const QString& ctx)
{
    static foobar foo;

    QCoreApplication::postEvent(&foo, new logevent(type, msg, ctx));
}

} // app
