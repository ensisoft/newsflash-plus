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

#define LOGTAG "eventlog"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QIcon>
#include <newsflash/warnpop.h>
#include "eventlog.h"
#include "debug.h"

namespace app
{

enum {
    EventLogBufferSize = 200
};

EventLog::EventLog() 
  : unfiltered_events_(EventLogBufferSize)
  , filtered_events_(EventLogBufferSize)
{}

EventLog::~EventLog()
{}

void EventLog::write(Event::Type type, const QString& msg, const QString& tag)
{
    const auto time  = QTime::currentTime();
    const auto event = Event {type, msg, tag, time };
    
    emit newEvent(event);    
    if (type == Event::Type::Note)
        return;
    
    // this is the buffer of all events.
    unfiltered_events_.push_front(event);

    // then we maintain another buffer which is the currently visible events
    if (type == Event::Type::Info) 
    {
        if (!filter_.ShowInfos) 
            return;
    }
    else if (type == Event::Type::Warning) 
    {
        if (!filter_.ShowWarnings) 
            return;
    }
    else if (type == Event::Type::Error) 
    {
        if (!filter_.ShowErrors) 
            return;
    }

    if (tag == "engine") 
    {
        if (!filter_.ShowModuleEng) 
            return;
    }
    else if (tag == "app") 
    { 
        if (!filter_.ShowModuleApp) 
            return;
    }
    else if (tag == "gui") 
    { 
        if (!filter_.ShowModuleGui) 
            return;
    }
    else 
    { 
        if (!filter_.ShowModuleOther) 
            return;
    } 
    
    if (filtered_events_.full())
    {
        const auto first = index(0, 0);
        const auto last  = index((int)filtered_events_.size(), 1);
        filtered_events_.push_front(event);
        emit dataChanged(first, last);
    }
    else
    {
        beginInsertRows(QModelIndex(), 0, 0);
        filtered_events_.push_front(event);
        endInsertRows();
    }    
}

void EventLog::clear()
{
    beginRemoveRows(QModelIndex(), 0, filtered_events_.size());
    unfiltered_events_.clear();
    filtered_events_.clear();
    endRemoveRows();
}

int EventLog::rowCount(const QModelIndex&) const
{
    return static_cast<int>(filtered_events_.size());
}

QVariant EventLog::data(const QModelIndex& index, int role) const
{
    const auto row = index.row();
    const auto& ev = filtered_events_[row];

    if (role == Qt::DecorationRole)
    {
        using type = Event::Type;
        switch (ev.type)
        {
            case type::Warning: return QIcon("icons:ico_warning.png");
            case type::Info:    return QIcon("icons:ico_info.png");
            case type::Error:   return QIcon("icons:ico_error.png");
            case type::Note:    return QIcon("icons:ico_note.png");
        }
    }
    if (role == Qt::DisplayRole)
    {
        return QString("[%1] [%2] %3")
            .arg(ev.time.toString("hh:mm:ss:zzz"))
            .arg(ev.logtag)
            .arg(ev.message);                        
    }
    return {};
}

EventLog& EventLog::get()
{
    static EventLog log;
    return log;
}

void EventLog::filter(const Filtering& filtering)
{
    boost::circular_buffer<Event> events(EventLogBufferSize);

    for (const auto& event : unfiltered_events_)
    {
        if (event.type == Event::Type::Info)
        {
            if (!filtering.ShowInfos) 
                continue;
        }
        else if (event.type == Event::Type::Warning)
        {
            if (!filtering.ShowWarnings) 
                continue;
        }
        else if (event.type == Event::Type::Error)
        {
            if (!filtering.ShowErrors)
                continue;
        }
        
        if (event.logtag == "engine") 
        {
            if (!filtering.ShowModuleEng) 
                continue;
        }
        else if (event.logtag == "app")
        { 
            if (!filtering.ShowModuleApp)
                continue;
        }
        else if (event.logtag == "gui")
        { 
            if (!filtering.ShowModuleGui) 
                continue;
        }
        else 
        {
            if (!filtering.ShowModuleOther) 
                continue;
        }   
        events.push_back(event);
    }
    QAbstractListModel::beginResetModel();
    filtered_events_ = std::move(events);
    QAbstractListModel::endResetModel();
    filter_ = filtering;
}

} // app
    