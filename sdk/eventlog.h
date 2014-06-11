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
#  include <boost/circular_buffer.hpp>
#  include <QAbstractListModel>
#  include <QEvent>
#  include <QTime>
#  include <QString>
#include <newsflash/warnpop.h>

#include "model.h"

namespace sdk
{
    // eventlog is technically a backend module to the coreapp and could
    // generally be a builtin. but since it's a universally handy module
    // it lives here in the sdk library.
    class eventlog : public sdk::model, public QAbstractListModel
    {
    public:
        enum class event {
            warning, info, error
        };

        // record a new event in the log
        void write(event type, const QString& msg, const QString& tag);

        // clear the event log
        virtual void clear() override;

        virtual QAbstractItemModel* view() override;

        virtual QString name() const override;

        // abstraclistmodel data accessor
        virtual int rowCount(const QModelIndex&) const override;

        // abstractlistmodel data accessor
        virtual QVariant data(const QModelIndex& index, int role) const override;

        static
        eventlog& get();

    private:
        eventlog();
       ~eventlog();

    private:
        struct event_t {
            event   type;
            QString message;
            QString logtag;
            QTime   time;
        };

        boost::circular_buffer<event_t> events_;
    };

#ifndef LOGTAG
    // we want every log event to be tracable back to where it came from
    // so thus every module should define it's own LOGTAG 
//#  error every module importing eventlog needs to define LOGTAG
#endif

#define WARN(m) \
    sdk::eventlog::get().write(sdk::eventlog::event::warning, m, LOGTAG)    

#define ERROR(m) \
    sdk::eventlog::get().write(sdk::eventlog::event::error, m, LOGTAG)    

#define INFO(m) \
    sdk::eventlog::get().write(sdk::eventlog::event::info, m, LOGTAG)    

} // sdk
