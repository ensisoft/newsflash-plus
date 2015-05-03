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

#define LOGTAG "news"

#include <newsflash/warnpush.h>
#  include <QtGui/QFont>
#  include <QtGui/QIcon>
#  include <QDir>
#  include <QStringList>
#  include <QFile>
#  include <QTextStream>
#  include <QFileInfo>
#include <newsflash/warnpop.h>
#include <algorithm>
#include "newslist.h"
#include "settings.h"
#include "eventlog.h"
#include "debug.h"
#include "format.h"
#include "engine.h"
#include "accounts.h"
#include "platform.h"
#include "types.h"
#include "newsinfo.h"
#include "utility.h"

namespace app
{

const int CurrentFileVersion = 1;

NewsList::NewsList() : sort_(Columns::LAST), order_(Qt::AscendingOrder), size_(0), account_(0)
{
    DEBUG("NewsList created");

    QObject::connect(g_engine, SIGNAL(listCompleted(quint32, const QList<app::NewsGroupInfo>&)),
        this, SLOT(listCompleted(quint32, const QList<app::NewsGroupInfo>&)));

    QObject::connect(g_engine, SIGNAL(newHeaderDataAvailable(const QString&)),
        this, SLOT(newHeaderDataAvailable(const QString&)));
}

NewsList::~NewsList()
{
    DEBUG("NewsList deleted");
}


QVariant NewsList::headerData(int section, Qt::Orientation orietantation, int role) const 
{
    if (orietantation != Qt::Horizontal)
        return QVariant();

    if (role == Qt::DisplayRole)
    {
        switch ((NewsList::Columns)section)
        {
            case Columns::Messages:   return "Messages";            
            case Columns::SizeOnDisk: return "Size";
            case Columns::Subscribed: return "";
            case Columns::Name:       return "Name";
            case Columns::LAST: Q_ASSERT(false); break;
        }
    }
    else if (role == Qt::DecorationRole)
    {
        static const QIcon gray(toGrayScale(QPixmap("icons:ico_favourite.png")));

        if ((Columns)section == Columns::Subscribed)
            return gray;

    }
    return QVariant();
}

QVariant NewsList::data(const QModelIndex& index, int role) const
{
    const auto col = index.column();
    const auto row = index.row();
    if (role == Qt::DisplayRole)
    {
        const auto& group = groups_[row];
        switch ((NewsList::Columns)col)
        {
            case Columns::Messages:    return toString(app::count{group.numMessages});
            case Columns::SizeOnDisk:  
                if (group.sizeOnDisk == 0)
                    break;
                return toString(app::size{group.sizeOnDisk});

            case Columns::Name:        return group.name;
            case Columns::Subscribed:  break;
            case Columns::LAST: Q_ASSERT(0); break;
        }
    }
    else if (role == Qt::DecorationRole)
    {
        const auto& group = groups_[row];        
        switch ((NewsList::Columns)col)
        {
            case Columns::Messages:
                return QIcon("icons:ico_news.png");
            case Columns::Subscribed:
                if (group.flags & Flags::Subscribed)
                    return QIcon("icons:ico_favourite.png");
                break;

            case Columns::Name: break;
            case Columns::SizeOnDisk: break;
            case Columns::LAST: Q_ASSERT(0); break;
        }
    }

    return {};
}

void NewsList::sort(int column, Qt::SortOrder order) 
{
    if ((Columns)column == Columns::Subscribed)
        return;

    DEBUG("Sorting by %1 into %2", column, order);

    emit layoutAboutToBeChanged();

    auto beg = std::begin(groups_);
    auto end = std::begin(groups_) + size_;
    switch ((Columns)column)
    {
        case Columns::Messages:  
            app::sort(beg, end, order, &group::numMessages);
            break;

        case Columns::SizeOnDisk:
            app::sort(beg, end, order, &group::sizeOnDisk);
            break;

        case Columns::Name:      
            app::sort(beg, end, order, &group::name); 
            break;

        default: Q_ASSERT("wut"); break;
    }

    // and put the subscribed items at the top.
    auto it = std::stable_partition(beg, end, [&](const group& g) {
            return ((g.flags & Flags::Subscribed) != 0);
        });    

    DEBUG("Found %1 favs", std::distance(beg, it)); 
    Q_UNUSED(it);

    emit layoutChanged();
    sort_  = (Columns)column;
    order_ = order;
}

int NewsList::rowCount(const QModelIndex&) const 
{
    return (int)size_;
}

int NewsList::columnCount(const QModelIndex&) const
{
    return (int)Columns::LAST;
}


void NewsList::clear()
{
    QAbstractTableModel::beginResetModel();

    groups_.clear();
    size_ = 0;

    QAbstractTableModel::reset();
    QAbstractTableModel::endResetModel();
}

void NewsList::loadListing(const QString& file, quint32 accountId)
{
    DEBUG("Loading newslist from %1", file);

    QFile io(file);
    if (!io.open(QIODevice::ReadOnly))
    {
        ERROR("Failed to read listing file %1", file, io.error());
        return;
    }

    QAbstractTableModel::beginResetModel();

    const auto* account  = g_accounts->findAccount(accountId);
    const auto& datapath = account->datapath;
    const auto& newslist = account->subscriptions;

    QTextStream stream(&io);
    stream.setCodec("UTF-8");

    /* const quint32 curVersion  = */ stream.readLine().toUInt();
    const quint32 numGroups  = stream.readLine().toUInt();

    quint32 curGroup   = 0;
    while (!stream.atEnd())
    {
        const auto& line = stream.readLine();
        const auto& toks = line.split("\t");
        group g;
        g.name  = toks[0];
        g.numMessages = toks[1].toULongLong();
        g.sizeOnDisk  = sumFileSizes(joinPath(datapath, g.name));
        g.flags = 0;
        for (int i=0; i<newslist.size(); ++i)
        {
            if (newslist[i] == g.name)
                g.flags |= Flags::Subscribed;
        }
        groups_.push_back(g);
        curGroup++;

        if (!(curGroup % 100))
            emit progressUpdated(accountId, numGroups, curGroup);
    }

    size_ = groups_.size();
    account_ = accountId;

    DEBUG("Loaded %1 items", size_);

    // note that the ordering of the data is undefined at this moment.
    // the UI should then perform sorting.

    QAbstractTableModel::reset();
    QAbstractTableModel::endResetModel();

    emit loadComplete(accountId);
}

void NewsList::makeListing(const QString& file, quint32 account)
{
    auto it = pending_.find(account);
    if (it != std::end(pending_))
        return;

    auto batchid = g_engine->retrieveNewsgroupListing(account);

    operation op;
    op.file    = file;
    op.account = account;
    op.batchId  = batchid;
    pending_.insert(std::make_pair(account, op));

    emit progressUpdated(account, 0, 0);
}

void NewsList::subscribe(QModelIndexList& list, quint32 accountId)
{
    int minIndex = std::numeric_limits<int>::max();
    int maxIndex = std::numeric_limits<int>::min();

    qSort(list);
    for (int i=0; i<list.size(); ++i)
    {
        const int row = list[0].row();
        if (row < minIndex)
            minIndex = row;
        if (row > maxIndex)
            maxIndex = row;

        auto& group = groups_[row];
        group.flags |= Flags::Subscribed;
    }

    auto first = QAbstractTableModel::index(minIndex, 0);
    auto last  = QAbstractTableModel::index(maxIndex, (int)Columns::LAST);
    emit dataChanged(first, last);

    setAccountSubscriptions(accountId);
}

void NewsList::unsubscribe(QModelIndexList& list, quint32 accountId)
{
    int minIndex = std::numeric_limits<int>::max();
    int maxIndex = std::numeric_limits<int>::min();

    qSort(list);
    for (int i=0; i<list.size(); ++i)
    {
        const int row = list[0].row();
        if (row < minIndex)
            minIndex = row;
        if (row > maxIndex)
            maxIndex = row;

        auto& group = groups_[row];
        group.flags &= ~Flags::Subscribed;
    }

    auto first = QAbstractTableModel::index(minIndex, 0);
    auto last  = QAbstractTableModel::index(maxIndex, (int)Columns::LAST);
    emit dataChanged(first, last);

    setAccountSubscriptions(accountId);    
}

void NewsList::clearSize(const QModelIndex& index)
{
    const auto row = index.row();
    BOUNDSCHECK(groups_, row);

    groups_[row].sizeOnDisk = 0;

    const auto first = QAbstractTableModel::index(row, 0);
    const auto last  = QAbstractTableModel::index(row, (int)Columns::LAST);
    emit dataChanged(first, last);
}

QString NewsList::getName(const QModelIndex& index) const 
{
    return groups_[index.row()].name;
}

QString NewsList::getName(std::size_t index) const 
{
    return groups_[index].name;
}

std::size_t NewsList::numItems() const 
{
    return size_;
}

void NewsList::filter(const QString& str, bool showEmpty)
{
    Q_ASSERT((sort_ != Columns::LAST) &&
        "The data is not yet sorted. Current sorting is needed for filtering.");

    DEBUG("Filter with %1", str);

    QAbstractTableModel::beginResetModel();

    auto beg = std::begin(groups_);
    auto end = std::partition(std::begin(groups_), std::end(groups_), 
        [&](const group& g) {
        if (!showEmpty && !g.numMessages)
            return false;
        return g.name.indexOf(str) != -1;
    });

    size_ = std::distance(beg, end);
    
    switch (sort_)
    {
        case Columns::Messages:
            app::sort(beg, end, order_, &group::numMessages);
            break;
        case Columns::Name:
            app::sort(beg, end, order_, &group::name);
            break;
        case Columns::SizeOnDisk:
            app::sort(beg, end, order_, &group::sizeOnDisk);
            break;
        default: Q_ASSERT(!"incorrect sorting");
    }

    // and finally grab the favs and put them at the top
    beg = std::begin(groups_);
    end = std::begin(groups_) + size_;
    auto it = std::stable_partition(beg, end, [&](const group& g) {
            return ((g.flags & Flags::Subscribed) != 0);
        });   

    DEBUG("Found %1 matching items...", size_);    
    DEBUG("Found %1 favs", std::distance(beg, it));
    Q_UNUSED(it);

    QAbstractTableModel::reset();
    QAbstractTableModel::endResetModel();
}

void NewsList::listCompleted(quint32 acc, const QList<app::NewsGroupInfo>& list)
{
    auto it = pending_.find(acc);
    ENDCHECK(pending_, it);

    auto op = it->second;

    QFile io(op.file);
    if (!io.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        ERROR("Unable to write listing file %1", op.file, io.error());
        return;
    }

    QTextStream stream(&io);
    stream.setCodec("UTF-8");
    stream << CurrentFileVersion << "\n";
    stream << list.size() << "\n";

    for (int i=0; i<list.size(); ++i)
    {
        const auto& group = list[i];
        stream << group.name << "\t" << group.size << "\n";
    }

    io.flush();
    io.close();

    pending_.erase(it);

    emit makeComplete(acc);
}

void NewsList::newHeaderDataAvailable(const QString& file)
{
    if (account_ == 0)
        return;

    const auto* account  = g_accounts->findAccount(account_);
    const auto& datapath = account->datapath;

    if (file.indexOf(datapath) == -1)
        return;

    auto it = std::find_if(std::begin(groups_), std::end(groups_),
        [&](const group& g) {
            if (file.indexOf(g.name) != -1)
                return true;
            return false;
        });
    if (it == std::end(groups_))
        return;

    auto& group = *it;

    group.sizeOnDisk = sumFileSizes(joinPath(datapath, group.name));

    DEBUG("%1 was updated. New size on disk is %2",
        group.name, app::size{group.sizeOnDisk});

    auto row = std::distance(std::begin(groups_), it);
    auto first = QAbstractTableModel::index(row, 0);
    auto last  = QAbstractTableModel::index(row, (int)Columns::LAST);
    emit dataChanged(first, last);
}

void NewsList::setAccountSubscriptions(quint32 accountId)
{
    QStringList list;
    for (const auto& group : groups_)
    {
        if (group.flags & Flags::Subscribed)
            list << group.name;
    }

    auto* account = g_accounts->findAccount(accountId);
    Q_ASSERT(account);

    account->subscriptions = list;
}

} // app
