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
#  include <QObject>
#  include <QEvent>
#  include <QTime>
#include <newsflash/warnpop.h>

class QCoreApplication;

namespace app
{
    class eventlog : public QObject
    {
        Q_OBJECT

    public:
        enum class event_t {
            debug, warning, info, error
        };

        struct event {
            event_t type;
            QString message;
            QString context;
            QTime   time;
        };

        eventlog();
       ~eventlog();

        void make_global(QCoreApplication* app);

        void write(event_t type, const QString& msg, const QString& ctx);

        void clear();

        const event& operator[](int i) const;

        std::size_t size() const;


        // post a log event
        static
        void post(event_t type, const QString& msg, const QString& ctx);

    signals:
        void insert_event(std::size_t pos);
        void update_event(std::size_t pos);
        void clear_events();

    private:
        class logevent : public QEvent
        {
        public:
            logevent(eventlog::event_t e,
                     const QString& wut, 
                     const QString ctx) : QEvent(type()),
            message_(wut), context_(ctx), type_(e)
            {
                time_ = QTime::currentTime();
            }

            static QEvent::Type type()
            {
                static QEvent::Type id = 
                    static_cast<QEvent::Type>(QEvent::registerEventType());
                return id;
            }
        private: 
            friend class eventlog;

        private:
            QString message_;
            QString context_;
            QTime   time_;
            eventlog::event_t type_;
        };

    private:
        bool eventFilter(QObject* object, QEvent* event);

    private:
        boost::circular_buffer<event> events_;

        QCoreApplication* app_;
    };


#define DEBUG(m) \
    app::eventlog::post(app::eventlog::event_t::debug, m, "default")

#define WARN(m) \
    app::eventlog::post(app::eventlog::event_t::warning, m, "default")    

#define ERROR(m) \
    app::eventlog::post(app::eventlog::event_t::error, m, "default")    

#define INFO(m) \
    app::eventlog::post(app::eventlog::event_t::info, m, "default")    


} // app
