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

#define LOGTAG "conns"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QIcon>
#include <newsflash/warnpop.h>

#include "connlist.h"
#include "engine.h"
#include "debug.h"
#include "format.h"
#include "types.h"

namespace {
    using states = newsflash::ui::connection::states;
    using errors = newsflash::ui::connection::errors;

    enum class Columns { 
        Status, Server, Data, Kbs, Desc, LAST
    };

    const char* str(states s)
    {
        switch (s)
        {
            case states::disconnected: return "Disconnected";
            case states::resolving:    return "Resolving";
            case states::connecting:   return "Connecting";
            case states::initializing: return "Authenticating";
            case states::connected:    return "Connected";
            case states::active:       return "Active";
            case states::error:        return "Error";
        }
        return "???";
    }

    const char* str(errors e)
    {
        switch (e)
        {
            case errors::none:                    return "None";
            case errors::resolve:                 return "Resolve";
            case errors::refused:                 return "Refused";
            case errors::authentication_rejected: return "Authentication Rejected";
            case errors::no_permission:           return "No Permission";
            case errors::network:                 return "Network Error";
            case errors::timeout:                 return "Timeout";
            case errors::other:                   return "Error";
        }
        return "???";
    }    
} // namespace

namespace app
{


ConnList::ConnList()
{}

ConnList::~ConnList()
{}

QVariant ConnList::data(const QModelIndex& index, int role) const
{
    const auto col = index.column();
    const auto row = index.row();
    const auto& ui = conns_[row];

    if (role == Qt::DisplayRole)
    {
        switch ((Columns)col)
        {
            case Columns::Status:
                if (ui.state == states::error)
                    return str(ui.error);
                return str(ui.state);

            case Columns::Server: return fromLatin(ui.host);
            case Columns::Data:   return toString(size{ui.down});
            case Columns::Kbs:    return toString(speed{ui.bps});
            case Columns::Desc:   return fromUtf8(ui.desc);
            case Columns::LAST:  Q_ASSERT(false);
        }
    }
    else if (role == Qt::DecorationRole && col == (int)Columns::Status)
    {
        switch (ui.state)
        {
            case states::disconnected:  return QIcon("icons:ico_conn_disconnected.png");
            case states::resolving:     return QIcon("icons:ico_conn_connecting.png");
            case states::connecting:    return QIcon("icons:ico_conn_connecting.png");
            case states::initializing:  return QIcon("icons:ico_conn_connecting.png");
            case states::connected:     return QIcon("icons:ico_conn_connected.png");
            case states::active:        return QIcon("icons:ico_conn_active.png");                                                                
            case states::error:         return QIcon("icons:ico_conn_error.png");                                                                                
        }
    }
    else if (role == Qt::DecorationRole && col == (int)Columns::Server)
    {
       return ui.secure ? 
            QIcon("icons:ico_lock.png") :
            QIcon("icons:ico_unlock.png");
    }
    return QVariant();
}

QVariant ConnList::headerData(int section, Qt::Orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return {};

    switch (Columns(section))
    {
        case Columns::Status: return "Status";
        case Columns::Server: return "Server";
        case Columns::Data:   return "Data";
        case Columns::Kbs:    return "Speed";
        case Columns::Desc:   return "Description";
        case Columns::LAST:   Q_ASSERT(false);
    }
    return {};
}

int ConnList::rowCount(const QModelIndex&) const
{
    return  (int)conns_.size();
}

int ConnList::columnCount(const QModelIndex&) const
{
    return (int)Columns::LAST;
}

void ConnList::refresh()
{
    //DEBUG("refresh");

    const auto cur_size = conns_.size();

    g_engine->refreshConnList(conns_);

    if (conns_.size() != cur_size)
    {
        reset();
    }
    else
    {
        auto first = QAbstractTableModel::index(0, 0);
        auto last  = QAbstractTableModel::index(conns_.size(), (int)Columns::LAST);
        emit dataChanged(first, last);
    }

}

void ConnList::kill(QModelIndexList& list)
{
    qSort(list);

    int removed = 0;

    for (int i=0; i<list.size(); ++i)
    {
        const auto row = list[i].row() - removed;
        beginRemoveRows(QModelIndex(), row, row);
        g_engine->killConnection(row);
        g_engine->refreshConnList(conns_);
        endRemoveRows();
        ++removed;
    }
}

void ConnList::clone(QModelIndexList& list)
{
    for (int i=0; i<list.size(); ++i)
    {
        const auto row = list[i].row();
        beginInsertRows(QModelIndex(), conns_.size(), conns_.size());
        g_engine->cloneConnection(row);
        g_engine->refreshConnList(conns_);
        endInsertRows();
    }
}

} // app
