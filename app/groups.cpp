// Copyright (c) 2014 Sami Väisänen, Ensisoft 
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

#define LOGTAG "app"

#include <newsflash/warnpush.h>
#  include <QtGui/QFont>
#  include <QtGui/QIcon>
#  include <QDir>
#  include <QStringList>
#  include <QFile>
#  include <QTextStream>
#  include <QFileInfo>
#include <newsflash/warnpop.h>

#include "settings.h"
#include "eventlog.h"
#include "debug.h"
#include "groups.h"
#include "format.h"
#include "engine.h"

namespace app
{

Groups::Groups()
{
    DEBUG("Groups created");

    QObject::connect(g_engine, SIGNAL(listingCompleted(quint32, const QList<app::NewsGroup>&)),
        this, SLOT(listingCompleted(quint32, const QList<app::NewsGroup>&)));
}

Groups::~Groups()
{
    DEBUG("Groups deleted");
}


QVariant Groups::headerData(int section, Qt::Orientation orietantation, int role) const 
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orietantation != Qt::Horizontal)
        return QVariant();

    switch ((Groups::column)section)
    {
        case column::name:
            return "Name";

        case column::updated:
            return "Updated";

        case column::headers:
            return "Headers";

        case column::articles:
            return "Articles";

        case column::size:
            return "Size";

        default:
            Q_ASSERT(!"missing column case");
            break;
    }
    return QVariant();
}

QVariant Groups::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        const auto col    = index.column();
        const auto row    = index.row();
        const auto& group = groups_[row];
        switch ((Groups::column)col)
        {
            case column::name:
               return group.name;

            case column::updated:
                return format(age{group.updated});

            case column::articles:
                //return group.articles;
                return 0;


            case column::size:
                return format(size{group.sizeOnDisk});

            default:
                Q_ASSERT(!"missing column case");
                break;
        }
    }

    return QVariant();
}

int Groups::rowCount(const QModelIndex&) const 
{
    return groups_.size();
}

int Groups::columnCount(const QModelIndex&) const
{
    return (int)column::last;
}


void Groups::clear()
{
    groups_.clear();
    reset();
    account_ = 0;
}

void Groups::loadListing(const QString& file, quint32 account)
{
    QFile io(file);
    if (!io.open(QIODevice::ReadOnly))
    {
        ERROR(str("Failed to read listing file _1", io));
        return;
    }

    QTextStream stream(&io);
    stream.setCodec("UTF-8");

    const quint32 numGroups = stream.readLine().toULong();
          quint32 numGroup  = 0;
    while (!stream.atEnd())
    {
        const auto& line = stream.readLine();
        const auto& toks = line.split("\t");
        group g;
        g.name = toks[0];
        g.updated = QDateTime::fromTime_t(toks[1].toLong());
        g.numMessages = toks[2].toULongLong();
        g.flags       = toks[3].toLong();
        groups_.push_back(g);
        ++numGroup;
    }
    reset();
}

void Groups::makeListing(const QString& file, quint32 account)
{
    auto it = pending_.find(account);
    if (it != std::end(pending_))
        return;

    auto taskid = g_engine->retrieveNewsgroupListing(account);

    operation op;
    op.file    = file;
    op.account = account;
    op.taskId  = taskid;
    pending_.insert(std::make_pair(account, op));
}

void Groups::listingCompleted(quint32 acc, const QList<app::NewsGroup>& list)
{
    auto it = pending_.find(acc);
    Q_ASSERT(it != std::end(pending_));

    auto op = it->second;

    std::map<QString, group> map;

    QFileInfo info(op.file);
    if (info.exists())
    {
        QFile io(op.file);
        if (!io.open(QIODevice::ReadOnly))
        {
            ERROR(str("Unable to read listing file _1", io));
            return;
        }
        QTextStream stream(&io);
        stream.setCodec("UTF-8");
        stream.readLine();
        while (!stream.atEnd())
        {
            const auto& line = stream.readLine();
            const auto& toks = line.split("\t");
            group g;
            g.name        = toks[0];
            g.updated     = QDateTime::fromTime_t(toks[1].toLong());
            g.numMessages = toks[2].toULongLong();
            g.flags       = toks[3].toLong();
            map.insert(std::make_pair(g.name, g));
        }
    }

    for (int i=0; i<list.size(); ++i)
    {
        const auto& group = list[i];
        auto it = map.find(group.name);
        if (it == std::end(map))
        {
            Groups::group g;
            g.name = group.name;
            g.numMessages = group.size;
            g.flags = 0;
            g.sizeOnDisk = 0;
            map.insert(std::make_pair(g.name, g));
        }
        else
        {
            auto& g = it->second;
            g.numMessages = group.size;
        }
    }


    QFile io(op.file);
    if (!io.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        ERROR(str("Unable to write listing file _1", io));
        return;
    }

    QTextStream stream(&io);
    stream.setCodec("UTF-8");
    stream << map.size() << "\n";

    for (const auto& pair : map)
    {
        const auto& group = pair.second;
        stream << group.name << "\t" << group.updated.toTime_t() << "\t" << group.numMessages << "\t" << group.flags;
        stream << "\n";
    }

    pending_.erase(it);
}

} // app
