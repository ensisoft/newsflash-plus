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
#include <newsflash/warnpop.h>

#include "datastore.h"
#include "eventlog.h"
#include "debug.h"
#include "groups.h"
#include "format.h"


namespace app
{

groups::groups()
{
    DEBUG("groups created");
}

groups::~groups()
{
    DEBUG("groups deleted");
}

void groups::save(datastore& values) const
{
    QStringList list;
    for (const auto& group : groups_)
    {
        const auto& key = group.name;
        values.set(key, "name", group.name);
        values.set(key, "updated", group.updated);
        values.set(key, "articles", group.articles);
        list.push_back(key);
    }
    values.set("groups", "list", list);    
}


void groups::load(const datastore& values) 
{

    const QStringList& list = values.get("groups", "list").toStringList();
    for (const auto& key : list)
    {
        groups::group group {};
        group.name     = values.get(key, "name").toString();
        group.updated  = values.get(key, "updated").toDateTime();
        group.headers  = 0;
        group.articles = values.get(key, "articles").toULongLong();
        group.size_on_disk = 0;

    }
}

// void groups::load_content()
// {
//     const auto listing = sdk::home::file("newsgroups.txt");
//     if (!QFile::exists(listing))
//     {

//     }

//     QFile io(listing);
//     if (!io.open(QIODevice::ReadOnly))
//     {
//         ERROR(str("Failed to open listing file _1, _2", io, io.error()));
//         return;
//     }

//     QTextStream stream(&io);
//     while (!stream.atEnd())
//     {
//         const auto& line = stream.readLine();
//     }


//     io.close();
// }

QAbstractItemModel* groups::view() 
{
    return this;
}

QVariant groups::headerData(int section, Qt::Orientation orietantation, int role) const 
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orietantation != Qt::Horizontal)
        return QVariant();

    switch ((groups::column)section)
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

QVariant groups::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        const auto col    = index.column();
        const auto row    = index.row();
        const auto& group = groups_[row];
        switch ((groups::column)col)
        {
            case column::name:
               return group.name;

            case column::updated:
                return format(age{group.updated});

            case column::articles:
                return group.articles;


            case column::size:
                return format(size{group.size_on_disk});

            default:
                Q_ASSERT(!"missing column case");
                break;
        }
    }

    return QVariant();
}

int groups::rowCount(const QModelIndex&) const 
{
    return groups_.size();
}

int groups::columnCount(const QModelIndex&) const
{
    return (int)column::last;
}

} // app
