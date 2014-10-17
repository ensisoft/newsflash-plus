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

#define LOGTAG "nzb"

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QtGui/QIcon>
#  include <QFile>
#  include <QAbstractListModel>
#include <newsflash/warnpop.h>

#include "nzbfile.h"
#include "nzbthread.h"
#include "debug.h"
#include "format.h"
#include "eventlog.h"
#include "filetype.h"


namespace app
{

nzbfile::nzbfile()
{
    DEBUG("nzbfile created");
}

nzbfile::~nzbfile()
{
    if (thread_.get())
    {
        DEBUG("Waiting nzb parser thread....");
        thread_->wait();
        thread_.reset();
    }

    DEBUG("nzbfile destroyed");    
}

bool nzbfile::load(const QString& file)
{
    std::unique_ptr<QFile> io(new QFile(file));
    if (!io->open(QIODevice::ReadOnly))
    {
        ERROR(str("Failed to open file _1", *io.get()));
        return false;
    }

    if (thread_.get())
    {
        DEBUG("Waiting existing parsing thread...");
        thread_->wait();
        thread_.reset();
    }
    thread_.reset(new nzbthread(std::move(io)));
    QObject::connect(thread_.get(), SIGNAL(complete()),
        this, SLOT(parse_complete()), Qt::QueuedConnection);
    thread_->start();
    return true;
}

void nzbfile::clear()
{
    data_.clear();
    QAbstractTableModel::reset();
}


int nzbfile::rowCount(const QModelIndex&) const
{
    return (int)data_.size();
}

int nzbfile::columnCount(const QModelIndex&) const
{
    return 3;
}

void nzbfile::sort(int column, Qt::SortOrder order)
{
    emit layoutAboutToBeChanged();
    if (column == 0)
    {
        std::sort(std::begin(data_), std::end(data_),
            [&](const nzbcontent& lhs, const nzbcontent& rhs) {
                if (order == Qt::AscendingOrder)
                    return lhs.type < rhs.type;
                return lhs.type > rhs.type;
            });
    }
    else if (column == 1)
    {
        std::sort(std::begin(data_), std::end(data_),
            [=](const nzbcontent& lhs, const nzbcontent& rhs) {
                if (order == Qt::AscendingOrder)
                    return lhs.bytes < rhs.bytes;
                return lhs.bytes > rhs.bytes;
            });
    }
    else if (column == 2)
    {
        std::sort(std::begin(data_), std::end(data_),
            [&](const nzbcontent& lhs, const nzbcontent& rhs) {
                if (order == Qt::AscendingOrder)
                    return lhs.subject < rhs.subject;
                return lhs.subject > rhs.subject;
            });
    }
    emit layoutChanged();
}

QVariant nzbfile::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        const auto& item = data_[index.row()];            
        if (index.column() == 0)
            return str(item.type);
        else if (index.column() == 1)
            return str(app::size { item.bytes });
        else if (index.column() == 2)
            return item.subject;
    }
    else if (role == Qt::DecorationRole)
    {
        if (index.column() == 0)
        {
            const auto& item = data_[index.row()];                            
            return iconify(item.type);
        }
    }
    return QVariant();
}

QVariant nzbfile::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (section == 0)
        return "Type";
    else if (section == 1)
        return "Size";
    else if (section == 2)
        return "File";

    return QVariant();
}

void nzbfile::parse_complete()
{
    DEBUG(str("parse_complete _1", QThread::currentThreadId()));

    const auto result = thread_->result(data_);

    switch (result)
    {
        case nzberror::none:
            DEBUG("NZB parse succesful!");
            break;
        case nzberror::xml:
            ERROR("XML error while parsing NZB file");
            break;

        case nzberror::nzb:
            ERROR("Error in NZB file content");
            break;

        case nzberror::io:
            ERROR("I/O error while parsing NZB file");
            break;

        case nzberror::other:
            ERROR("Unknown error while parsing NZB file");
            break;
    }

    reset();

    on_ready();
}


} // app
