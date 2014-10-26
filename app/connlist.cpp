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

#define LOGTAG "conns"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QIcon>
#include <newsflash/warnpop.h>

#include "connlist.h"
#include "engine.h"
#include "debug.h"
#include "format.h"

namespace app
{

using states = newsflash::ui::connection::states;
using errors = newsflash::ui::connection::errors;

const char* str(states s)
{
    switch (s)
    {
        case states::disconnected: return "Disconnected";
        case states::resolving: return "Resolving";
        case states::connecting: return "Connecting";
        case states::initializing: return "Authenticating";
        case states::connected: return "Connected";
        case states::active: return "Active";
        case states::error: return "Error";
    }
    return "???";
}

const char* str(errors e)
{
    switch (e)
    {
        case errors::none: return "None";
        case errors::resolve: return "Resolve";
        case errors::refused: return "Refused";
        case errors::authentication_rejected: return "Authentication Rejected";
        case errors::no_permission: return "No Permission";
        case errors::network: return "Network Error";
        case errors::timeout: return "Timeout";
        case errors::other: return "Error";
    }
    return "???";
}

connlist::connlist()
{}

connlist::~connlist()
{}

QVariant connlist::data(const QModelIndex& index, int role) const
{
    const auto col = index.column();
    const auto row = index.row();
    const auto& ui = *conns_[row];

    if (role == Qt::DisplayRole)
    {
        switch ((columns)col)
        {
            case columns::status:
                if (ui.state == states::error)
                    return str(ui.error);
                return str(ui.state);

            case columns::server:
                return from_utf8(ui.host);

            case columns::data:
                return format(size{ui.down});

            case columns::kbs:
                return format(speed{ui.bps});

            case columns::desc:
                return from_utf8(ui.desc);

            case columns::sentinel:
                Q_ASSERT(false);
        }
    }
    else if (role == Qt::DecorationRole && col == (int)columns::status)
    {
        switch (ui.state)
        {
            case states::disconnected: 
                return QIcon(":/resource/16x16_ico_png/ico_conn_disconnected.png");
            case states::resolving: 
                return QIcon(":/resource/16x16_ico_png/ico_conn_connecting.png");
            case states::connecting: 
                return QIcon(":/resource/16x16_ico_png/ico_conn_connecting.png");
            case states::initializing: 
                return QIcon(":/resource/16x16_ico_png/ico_conn_connecting.png");
            case states::connected:
                return QIcon(":/resource/16x16_ico_png/ico_conn_connected.png");
            case states::active: 
                return QIcon(":/resource/16x16_ico_png/ico_conn_active.png");                                                                
            case states::error: 
                return QIcon(":/resource/16x16_ico_png/ico_conn_error.png");                                                                                
        }
    }
    else if (role == Qt::DecorationRole && col == (int)columns::server)
    {
       return ui.secure ? 
            QIcon(":/resource/16x16_ico_png/ico_lock.png") :
            QIcon(":/resource/16x16_ico_png/ico_unlock.png");
    }
    return QVariant();
}

QVariant connlist::headerData(int section, Qt::Orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (columns(section))
    {
        case columns::status:
            return "Status";
        case columns::server:
            return "Server";
        case columns::data:
            return "Data";
        case columns::kbs:
            return "Speed";
        case columns::desc:
            return "Description";
        default:
            Q_ASSERT(!"incorrect column");
            break;
    }
    return QVariant();
}

int connlist::rowCount(const QModelIndex&) const
{
    return  (int)conns_.size();
}

int connlist::columnCount(const QModelIndex&) const
{
    return (int)columns::sentinel;
}

void connlist::refresh()
{
    //DEBUG("refresh");

    const auto cur_size = conns_.size();

    g_engine->update_conn_list(conns_);

    if (conns_.size() != cur_size)
    {
        reset();
    }
    else
    {
        auto first = QAbstractTableModel::index(0, 0);
        auto last  = QAbstractTableModel::index(conns_.size(), (int)columns::sentinel);
        emit dataChanged(first, last);
    }

}

void connlist::kill(QModelIndexList& list)
{
    qSort(list);

    int removed = 0;

    for (int i=0; i<list.size(); ++i)
    {
        const auto row = list[i].row() - removed;
        beginRemoveRows(QModelIndex(), row, row);
        g_engine->kill_connection(row);
        g_engine->update_conn_list(conns_);
        endRemoveRows();
        ++removed;
    }
}

void connlist::clone(QModelIndexList& list)
{
    for (int i=0; i<list.size(); ++i)
    {
        const auto row = list[i].row();
        beginInsertRows(QModelIndex(), conns_.size(), conns_.size());
        g_engine->clone_connection(row);
        g_engine->update_conn_list(conns_);
        endInsertRows();
    }
}

} // app
