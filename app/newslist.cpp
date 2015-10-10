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
#include "media.h"

namespace app
{

const int CurrentFileVersion = 2;
// FileVersion 2 adds the media type for the group. (MediaTypeVersion 2)

NewsList::NewsList() : sort_(Columns::LAST), order_(Qt::AscendingOrder), size_(0), account_(0)
{
    DEBUG("NewsList created");

    QObject::connect(g_engine, SIGNAL(listCompleted(quint32, const QList<app::NewsGroupInfo>&)),
        this, SLOT(listCompleted(quint32, const QList<app::NewsGroupInfo>&)));

    QObject::connect(g_engine, SIGNAL(newHeaderDataAvailable(const QString&, const QString&)),
        this, SLOT(newHeaderDataAvailable(const QString&, const QString&)));
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
            case Columns::Category:   return "Category";    
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
            case Columns::Messages:  
                return toString(app::count{group.numMessages});

            case Columns::Category:  
                return toString(group.type);

            case Columns::SizeOnDisk:  
                if (group.sizeOnDisk == 0)
                    break;
                return toString(app::size{group.sizeOnDisk});

            case Columns::Name:        
                return group.name;
            case Columns::Subscribed:  
                break;

            case Columns::LAST: 
                Q_ASSERT(0); 
                break;
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
                {
                    return QIcon("icons:ico_favourite.png");
                }
                break;

            case Columns::Category: break;
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
            app::sort(beg, end, order, &NewsGroup::numMessages);
            break;

        case Columns::Category:
            app::sort(beg, end, order, &NewsGroup::type);
            break;

        case Columns::SizeOnDisk:
            app::sort(beg, end, order, &NewsGroup::sizeOnDisk);
            break;

        case Columns::Name:      
            app::sort(beg, end, order, &NewsGroup::name); 
            break;

        default: Q_ASSERT("wut"); break;
    }

    // and put the subscribed items at the top.
    auto it = std::stable_partition(beg, end, [&](const NewsGroup& group) {
            return ((group.flags & Flags::Subscribed) != 0);
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

void NewsList::stop(quint32 account)
{
    auto it = pending_.find(account);
    if (it == std::end(pending_))
        return;    

    auto& op = it->second;
    g_engine->killAction(op.taskId);

    pending_.erase(it);
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

    const quint32 fileVersion = stream.readLine().toUInt();
    const quint32 numGroups   = stream.readLine().toUInt();

    DEBUG("Data file version %1", fileVersion);

    quint32 curGroup   = 0;
    while (!stream.atEnd())
    {
        const auto& line = stream.readLine();
        const auto& toks = line.split("\t");
        if (toks.size() < 2)
        {
            ERROR("Invalid newslist data.");
            return;
        }

        NewsGroup group;
        group.name  = toks[0];
        group.numMessages = toks[1].toULongLong();
        group.sizeOnDisk  = sumFileSizes(joinPath(datapath, group.name));
        group.flags       = 0;
        group.type        = MediaType::Other;

        if (fileVersion == 1)
        {
            group.type = findMediaType(group.name);
        }
        else if (fileVersion == 2)
        {
            if (toks.size() < 3)
            {
                ERROR("Invalid newslist data.");
                return;
            }

            // MediaTypeVersion 2
            group.type = (MediaType)toks[2].toInt();
        }

        for (int i=0; i<newslist.size(); ++i)
        {
            if (newslist[i] == group.name)
                group.flags |= Flags::Subscribed;
        }
        groups_.push_back(group);
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

    auto task = g_engine->retrieveNewsgroupListing(account);

    operation op;
    op.file    = file;
    op.account = account;
    op.taskId  = task;
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

void NewsList::filter(const QString& str, newsflash::bitflag<FilterFlags> options)
{
    Q_ASSERT((sort_ != Columns::LAST) &&
        "The data is not yet sorted. Current sorting is needed for filtering.");

    DEBUG("Filter with %1", str);

    QAbstractTableModel::beginResetModel();

    auto beg = std::begin(groups_);
    auto end = std::partition(std::begin(groups_), std::end(groups_), 
        [&](const NewsGroup& group) {

        if (isMusic(group.type) && !options.test(FilterFlags::ShowMusic))
            return false;

        if (isMovie(group.type) && !options.test(FilterFlags::ShowMovies))
            return false;

        if (isTelevision(group.type) && !options.test(FilterFlags::ShowTv))
            return false;

        if (isConsole(group.type) && !options.test(FilterFlags::ShowGames))
            return false;

        if (isApps(group.type) && !options.test(FilterFlags::ShowApps))
            return false;

        if (isAdult(group.type) && !options.test(FilterFlags::ShowAdult))
            return false;

        if (isImage(group.type) && !options.test(FilterFlags::ShowImages))
            return false;

        if (isOther(group.type) && !options.test(FilterFlags::ShowOther))
            return false;

        if (!options.test(FilterFlags::ShowEmpty) && !group.numMessages)
            return false;

        if (!options.test(FilterFlags::ShowText))
        {
            if (!group.name.contains(".binaries."))
                return false;
        }

        return (bool)group.name.contains(str);
    });

    size_ = std::distance(beg, end);
    
    switch (sort_)
    {
        case Columns::Messages:
            app::sort(beg, end, order_, &NewsGroup::numMessages);
            break;
        case Columns::Name:
            app::sort(beg, end, order_, &NewsGroup::name);
            break;
        case Columns::SizeOnDisk:
            app::sort(beg, end, order_, &NewsGroup::sizeOnDisk);
            break;
        case Columns::Category:
            app::sort(beg, end, order_, &NewsGroup::type);
            break;

        default: Q_ASSERT(!"incorrect sorting");
    }

    // and finally grab the favs and put them at the top
    beg = std::begin(groups_);
    end = std::begin(groups_) + size_;
    auto it = std::stable_partition(beg, end, [&](const NewsGroup& group) {
            return ((group.flags & Flags::Subscribed) != 0);
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
        // in v4.0.0 we only write this data here.
        //stream << group.name << "\t" << group.size << "\n";
        // now we're adding the cached category for the group.
        const auto type = findMediaType(group.name);
        stream << group.name << "\t" << group.size << "\t" << (int)type << "\n";
    }

    io.flush();
    io.close();

    pending_.erase(it);

    emit makeComplete(acc);
}

void NewsList::newHeaderDataAvailable(const QString& newsGroupName, const QString& file)
{
    if (account_ == 0)
        return;

    const auto* account  = g_accounts->findAccount(account_);
    const auto& datapath = account->datapath;

    if (file.indexOf(datapath) == -1)
        return;

    auto it = std::find_if(std::begin(groups_), std::end(groups_),
        [&](const NewsGroup& group) {
            return group.name == newsGroupName;
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
