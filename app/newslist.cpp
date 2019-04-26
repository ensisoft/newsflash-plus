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

#define LOGTAG "newslist"

#include "newsflash/warnpush.h"
#  include <QtGui/QFont>
#  include <QtGui/QIcon>
#  include <QDir>
#  include <QStringList>
#  include <QFile>
#  include <QTextStream>
#  include <QFileInfo>
#include "newsflash/warnpop.h"

#include <algorithm>

#include "homedir.h"
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
#include "fileinfo.h"
#include "utility.h"
#include "media.h"

namespace app
{

const int CurrentFileVersion = 3;
// FileVersion 2 adds the media type for the group. (MediaTypeVersion 2)
// FileVersion 3 adds the last remote message number

NewsList::NewsList()
{
    DEBUG("NewsList created");

    QObject::connect(g_engine, SIGNAL(listCompleted(quint32, const QList<app::NewsGroupInfo>&)),
        this, SLOT(listCompleted(quint32, const QList<app::NewsGroupInfo>&)));
    QObject::connect(g_engine, SIGNAL(listUpdated(quint32, const QList<app::NewsGroupInfo>&)),
        this, SLOT(listUpdated(quint32, const QList<app::NewsGroupInfo>&)));
    QObject::connect(g_engine, SIGNAL(newHeaderInfoAvailable(const app::HeaderUpdateInfo&)),
        this, SLOT(newHeaderDataAvailable(const app::HeaderUpdateInfo&)));
    QObject::connect(g_engine, SIGNAL(updateCompleted(const app::HeaderInfo&)),
        this, SLOT(groupUpdateComplete(const app::HeaderInfo&)));

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
            case Columns::AllMessages:  return "Messages";
            case Columns::NewMessages:  return "New Messages";
            case Columns::Category:   return "Category";
            case Columns::SizeOnDisk: return "Size";
            case Columns::Favorite:   return "";
            case Columns::Name:       return "Name";
            case Columns::LAST: Q_ASSERT(false); break;
        }
    }
    else if (role == Qt::DecorationRole)
    {
        static const QIcon grayFav(toGrayScale(QPixmap("icons:ico_favourite.png")));
        if ((Columns)section == Columns::Favorite)
            return grayFav;
    }
    return QVariant();
}

QVariant NewsList::data(const QModelIndex& index, int role) const
{
    const auto col = index.column();
    const auto row = index.row();
    if (role == Qt::DisplayRole)
    {
        const auto& group = mCurrentGroupList[row];
        switch ((NewsList::Columns)col)
        {
            case Columns::AllMessages:
                return toString(app::count{group.numMessages});

            case Columns::NewMessages:
                if (group.sizeOnDisk) {
                    if (group.lastLocalMessageNumber == 0)
                        return tr("Please refresh group ...");
                    if (group.lastRemoteMessageNumber > group.lastLocalMessageNumber)
                        return toString(app::count{group.lastRemoteMessageNumber - group.lastLocalMessageNumber});
                }
                break;

            case Columns::Category:
                return toString(group.type);

            case Columns::SizeOnDisk:
                if (group.sizeOnDisk == 0)
                    break;
                return toString(app::size{group.sizeOnDisk});

            case Columns::Name:
                return group.name;
            case Columns::Favorite:
                break;

            case Columns::LAST:
                Q_ASSERT(0);
                break;
        }
    }
    else if (role == Qt::DecorationRole)
    {
        const auto& group = mCurrentGroupList[row];
        switch ((NewsList::Columns)col)
        {
            case Columns::AllMessages:
                return QIcon("icons:ico_news.png");

            case Columns::NewMessages:
                if (group.lastLocalMessageNumber != 0) {
                    if (group.lastRemoteMessageNumber > group.lastLocalMessageNumber)
                        return QIcon("icons:ico_new.png");
                }
                break;

            case Columns::Favorite:
                if (group.flags & Flags::Favorite)
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
    if ((Columns)column == Columns::Favorite)
        return;
    if ((Columns)column == Columns::NewMessages)
        return;

    DEBUG("Sorting by %1 into %2", column, order);

    emit layoutAboutToBeChanged();

    auto beg = std::begin(mCurrentGroupList);
    auto end = std::begin(mCurrentGroupList) + mNumCurrentlyVisible;
    switch ((Columns)column)
    {
        case Columns::AllMessages:
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

    // and put the favorite items at the top.
    auto it = std::stable_partition(beg, end, [&](const NewsGroup& group) {
            return ((group.flags & Flags::Favorite) != 0);
        });

    DEBUG("Found %1 favs", std::distance(beg, it));
    Q_UNUSED(it);

    emit layoutChanged();
}

int NewsList::rowCount(const QModelIndex&) const
{
    return static_cast<int>(mNumCurrentlyVisible);
}

int NewsList::columnCount(const QModelIndex&) const
{
    return static_cast<int>(Columns::LAST);
}

void NewsList::clear()
{
    QAbstractTableModel::beginResetModel();

    mCurrentGroupList.clear();
    mNumCurrentlyVisible = 0;

    QAbstractTableModel::reset();
    QAbstractTableModel::endResetModel();
}

void NewsList::stopRefresh(quint32 accountId)
{
    auto it = mPendingRefreshes.find(accountId);
    if (it == std::end(mPendingRefreshes))
        return;

    auto& op = it->second;
    const auto list = std::move(op.intermediateList);
    const auto file = op.file;
    g_engine->killTaskById(op.taskId);

    mPendingRefreshes.erase(it);

    if (list.empty())
        return;

    QFile io(file);
    if (!io.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        ERROR("Unable to write listing file %1", file, io.error());
        return;
    }

    QTextStream stream(&io);
    stream.setCodec("UTF-8");
    stream << CurrentFileVersion << "\n";
    stream << list.size() << "\n";
    for (const auto& group : list)
    {
        stream << group.name << "\t"
               << group.numMessages << "\t"
               << static_cast<int>(group.type) << "\t"
               << group.lastRemoteMessageNumber << "\n";
    }
    io.flush();
    io.close();

}

void NewsList::makeListing(quint32 accountId)
{
    auto it = mPendingRefreshes.find(accountId);
    if (it != std::end(mPendingRefreshes))
        return;

    const auto* account = g_accounts->findAccount(accountId);
    const auto& listing = app::homedir::file(account->name + ".lst");

    DEBUG("Refresh newslist '%1' '%2'", account->name, listing);

    const auto task = g_engine->retrieveNewsgroupListing(accountId);

    RefreshListAction refresh;
    refresh.file    = listing;
    refresh.account = accountId;
    refresh.taskId  = task;
    mPendingRefreshes.insert(std::make_pair(accountId, refresh));
}

void NewsList::loadListing(quint32 accountId)
{
    std::vector<NewsGroup> list;

    auto it = mPendingRefreshes.find(accountId);
    if (it != std::end(mPendingRefreshes))
    {
        DEBUG("Loading a intermediate list");

        const auto& refresh = it->second;
        list = refresh.intermediateList;
    }
    else
    {
        const auto* account     = g_accounts->findAccount(accountId);
        const auto& datapath    = account->datapath;
        const auto& favorites  = account->favorites;
        const auto& listingfile = app::homedir::file(account->name + ".lst");

        DEBUG("Loading newslist from %1", listingfile);

        QFileInfo listing_file_info(listingfile);
        QFile io(listingfile);
        if (!io.open(QIODevice::ReadOnly))
        {
            ERROR("Failed to read listing file %1", listingfile, io.error());
            return;
        }

        QTextStream stream(&io);
        stream.setCodec("UTF-8");

        const quint32 fileVersion = stream.readLine().toUInt();
        const quint32 numGroups   = stream.readLine().toUInt();

        DEBUG("Data file version %1", fileVersion);

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
            group.name        = toks[0];
            group.numMessages = toks[1].toULongLong();
            group.sizeOnDisk  = sumFileSizes(joinPath(datapath, group.name));
            group.flags       = 0;
            group.type        = MediaType::Other;

            if (fileVersion == 1)
            {
                group.type = findMediaType(group.name);
            }
            if (fileVersion >= 2)
            {
                if (toks.size() < 3)
                {
                    ERROR("Invalid newslist data.");
                    return;
                }
                group.type = (MediaType)toks[2].toInt();
            }
            if (fileVersion >= 3)
            {
                if (toks.size() < 4)
                {
                    ERROR("Invalid newslist data.");
                    return;
                }
                group.lastRemoteMessageNumber = toks[3].toULongLong();
            }

            // this file is written as a response to group update signal.
            // it has simple key=value format and contains the
            // local and remote first and last article numbers.
            // if this file is newer than the current listing file
            // then we use this information in the newsgroup list
            // instead of the information from the listing file itself.
            const QString& txt_file = joinPath(joinPath(datapath, group.name), group.name + ".txt");
            const QFileInfo info(txt_file);
            if (info.exists())
            {
                QMap<QString, QVariant> kvmap;
                if (!readKeyValueMap(txt_file, &kvmap))
                {
                    WARN("Failed to read group meta data file: %1", txt_file);
                }
                else
                {
                    const auto& lm_group_data = info.lastModified();
                    const auto& lm_list_file = listing_file_info.lastModified();
                    if (lm_group_data > lm_list_file) {
                        const auto remote_first = kvmap["remote_first"].toULongLong();
                        const auto remote_last  = kvmap["remote_last"].toULongLong();
                        group.numMessages = remote_last - remote_first + 1;
                        group.lastRemoteMessageNumber = remote_last;
                    }
                    group.lastLocalMessageNumber = kvmap["local_last"].toULongLong();
                }
            }

            for (const auto& favorite : favorites)
            {
                if (favorite == group.name) {
                    group.flags |= Flags::Favorite;
                    break;
                }
            }

            list.push_back(group);

            if ((list.size() % 100) == 0)
            {
                emit progressUpdated(accountId, numGroups, list.size());
            }
        }
    }

    // after the grouplist is reset:
    // - the filtering is now undefined.
    // - the sorting is now undefined.
    // the UI should do these operations again, it has the state

    QAbstractTableModel::beginResetModel();
    mCurrentGroupList    = std::move(list);
    mNumCurrentlyVisible = mCurrentGroupList.size();
    mCurrentAccountId    = accountId;
    QAbstractTableModel::reset();
    QAbstractTableModel::endResetModel();

    DEBUG("Loaded %1 items", mNumCurrentlyVisible);

    emit loadComplete(accountId);
}


void NewsList::markItemsFavorite(QModelIndexList& list)
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

        auto& group = mCurrentGroupList[row];
        group.flags |= Flags::Favorite;
    }

    auto first = QAbstractTableModel::index(minIndex, 0);
    auto last  = QAbstractTableModel::index(maxIndex, (int)Columns::LAST);
    emit dataChanged(first, last);

    setAccountFavorites();
}

void NewsList::clearFavoriteMarkFromItems(QModelIndexList& list)
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

        auto& group = mCurrentGroupList[row];
        group.flags &= ~Flags::Favorite;
    }

    auto first = QAbstractTableModel::index(minIndex, 0);
    auto last  = QAbstractTableModel::index(maxIndex, (int)Columns::LAST);
    emit dataChanged(first, last);

    setAccountFavorites();
}

void NewsList::toggleFavoriteMark(QModelIndexList& list)
{
    int minIndex = std::numeric_limits<int>::max();
    int maxIndex = std::numeric_limits<int>::min();

    qSort(list);
    for (int i=0; i<list.size(); ++i)
    {
        const int row = list[i].row();
        if (row < minIndex)
            minIndex = row;
        if (row > maxIndex)
            maxIndex = row;

        auto& group = mCurrentGroupList[row];
        group.flags ^= Flags::Favorite;
    }

    auto first = QAbstractTableModel::index(minIndex, 0);
    auto last  = QAbstractTableModel::index(maxIndex, (int)Columns::LAST);
    emit dataChanged(first, last);

    setAccountFavorites();
}

void NewsList::clearSize(const QModelIndex& index)
{
    const auto row = index.row();
    BOUNDSCHECK(mCurrentGroupList, row);

    mCurrentGroupList[row].sizeOnDisk = 0;
    mCurrentGroupList[row].lastLocalMessageNumber = 0;

    const auto first = QAbstractTableModel::index(row, 0);
    const auto last  = QAbstractTableModel::index(row, (int)Columns::LAST);
    emit dataChanged(first, last);
}

bool NewsList::isUpdating(quint32 account) const
{
    auto it = mPendingRefreshes.find(account);
    return it != mPendingRefreshes.end();
}

bool NewsList::hasListing(quint32 accountId) const
{
    const auto* account = g_accounts->findAccount(accountId);
    const auto& listing = app::homedir::file(account->name + ".lst");
    return QFile::exists(listing);
}

bool NewsList::hasData(const QModelIndex& index) const
{
    return mCurrentGroupList[index.row()].sizeOnDisk != 0;
}

bool NewsList::isFavorite(const QModelIndex& index) const
{
    return (mCurrentGroupList[index.row()].flags & Flags::Favorite) != 0;
}

QString NewsList::getName(const QModelIndex& index) const
{
    return mCurrentGroupList[index.row()].name;
}

QString NewsList::getName(std::size_t index) const
{
    return mCurrentGroupList[index].name;
}

MediaType NewsList::getMediaType(const QModelIndex& index) const
{
    return mCurrentGroupList[index.row()].type;
}

std::size_t NewsList::numItems() const
{
    return mNumCurrentlyVisible;
}

void NewsList::filter(const QString& str, newsflash::bitflag<FilterFlags> flags)
{
    DEBUG("Filter with %1", str);

    QAbstractTableModel::beginResetModel();

    auto beg = std::begin(mCurrentGroupList);
    auto end = std::partition(std::begin(mCurrentGroupList), std::end(mCurrentGroupList),
        [&](const NewsGroup& group) {
            if (isMusic(group.type) && !flags.test(FilterFlags::ShowMusic))
                return false;

            if (isMovie(group.type) && !flags.test(FilterFlags::ShowMovies))
                return false;

            if (isTelevision(group.type) && !flags.test(FilterFlags::ShowTv))
                return false;

            if (isConsole(group.type) && !flags.test(FilterFlags::ShowGames))
                return false;

            if (isApps(group.type) && !flags.test(FilterFlags::ShowApps))
                return false;

            if (isAdult(group.type) && !flags.test(FilterFlags::ShowAdult))
                return false;

            if (isImage(group.type) && !flags.test(FilterFlags::ShowImages))
                return false;

            if (isOther(group.type) && !flags.test(FilterFlags::ShowOther))
                return false;

            if (!flags.test(FilterFlags::ShowEmpty) && !group.numMessages)
                return false;

            if (!flags.test(FilterFlags::ShowText))
            {
                if (!group.name.contains(".binaries."))
                    return false;
            }

            return (bool)group.name.contains(str);
        });

    mNumCurrentlyVisible = std::distance(beg, end);

    DEBUG("Found %1 matching items...", mNumCurrentlyVisible);

    QAbstractTableModel::reset();
    QAbstractTableModel::endResetModel();
}

void NewsList::listCompleted(quint32 acc, const QList<app::NewsGroupInfo>& list)
{
    auto it = mPendingRefreshes.find(acc);
    ENDCHECK(mPendingRefreshes, it);

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
        stream <<  group.name << "\t"
                << group.size << "\t"
                << static_cast<int>(type) << "\t"
                << group.last << "\n";
    }

    io.flush();
    io.close();

    mPendingRefreshes.erase(it);

    emit makeComplete(acc);
}

void NewsList::listUpdated(quint32 accountId, const QList<app::NewsGroupInfo>& list)
{
    auto it = mPendingRefreshes.find(accountId);
    if (it == std::end(mPendingRefreshes))
        return;

    auto& op = it->second;

    const auto* account   = g_accounts->findAccount(accountId);
    const auto& favorites = account->favorites;
    const auto& datapath  = account->datapath;

    for (const auto& group : list)
    {
        NewsGroup next;
        next.name        = group.name;
        next.numMessages = group.size;
        next.lastRemoteMessageNumber = group.last;
        next.flags       = 0;
        next.type        = findMediaType(next.name);
        next.sizeOnDisk  = sumFileSizes(joinPath(datapath, next.name));

        for (const auto& favorite : favorites)
        {
            if (favorite == next.name) {
                next.flags |= Flags::Favorite;
                break;
            }
        }
        op.intermediateList.push_back(next);
    }

    emit listUpdate(accountId);
}

void NewsList::groupUpdateComplete(const app::HeaderInfo& info)
{
    const auto* account = g_accounts->findAccount(info.account);
    if (!account)
        return;
    const auto& name = info.groupName;
    const auto& path = joinPath(account->datapath, name);
    const auto& file = joinPath(path, name + ".txt");

    QFile io(file);
    if (!io.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        ERROR("Failed to open %1 for writing.", file);
        return;
    }
    QTextStream stream(&io);
    stream.setCodec("UTF-8");
    stream << "remote_last=" << info.remoteLast << "\n";
    stream << "remote_first=" << info.remoteFirst << "\n";
    stream << "local_first=" << info.localFirst << "\n";
    stream << "local_last=" << info.localLast << "\n";
    stream.flush();
    io.flush();
    io.close();
    DEBUG("Wrote group information in %1", file);

    if (info.account != mCurrentAccountId)
        return;
    auto it = std::find_if(std::begin(mCurrentGroupList), std::end(mCurrentGroupList),
        [&](const NewsGroup& group) {
            return group.name == info.groupName;
        });
    if (it == std::end(mCurrentGroupList))
        return;

    auto& group = *it;
    group.sizeOnDisk = sumFileSizes(joinPath(account->datapath, info.groupName));
    group.lastLocalMessageNumber = info.localLast;
    group.lastRemoteMessageNumber = info.remoteLast;
    group.numMessages = info.numRemoteArticles;

    auto row = std::distance(std::begin(mCurrentGroupList), it);
    auto first = QAbstractTableModel::index(row, 0);
    auto last  = QAbstractTableModel::index(row, (int)Columns::LAST);
    emit dataChanged(first, last);

}

void NewsList::newHeaderDataAvailable(const app::HeaderUpdateInfo& info)
{
    const auto* account  = g_accounts->findAccount(info.account);
    if (!account)
        return;

    const auto& datapath = account->datapath;
    const auto& newsGroupName = info.groupName;
    const auto& newsGroupFile = info.groupFile;
    const auto& path = joinPath(datapath, info.groupName);
    const auto& file = joinPath(path, info.groupName + ".txt");

    QFile io(file);
    if (!io.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        ERROR("Failed to open %1 for writing.", file);
        return;
    }
    QTextStream stream(&io);
    stream.setCodec("UTF-8");
    stream << "remote_last=" << info.remoteLast << "\n";
    stream << "remote_first=" << info.remoteFirst << "\n";
    stream << "local_first=" << info.localFirst << "\n";
    stream << "local_last=" << info.localLast << "\n";
    stream.flush();
    io.flush();
    io.close();
    DEBUG("Wrote group information in %1", file);

    if (info.account != mCurrentAccountId)
        return;

    // the fuck is this check?
    if (newsGroupFile.indexOf(datapath) == -1)
        return;

    auto it = std::find_if(std::begin(mCurrentGroupList), std::end(mCurrentGroupList),
        [&](const NewsGroup& group) {
            return group.name == newsGroupName;
        });
    if (it == std::end(mCurrentGroupList))
        return;

    auto& group = *it;

    group.sizeOnDisk = sumFileSizes(joinPath(datapath, group.name));
    group.lastLocalMessageNumber  = info.localLast;
    group.lastRemoteMessageNumber = info.remoteLast;
    group.numMessages = info.numRemoteArticles;

    DEBUG("%1 was updated. New size on disk is %2",
        group.name, app::size{group.sizeOnDisk});

    auto row = std::distance(std::begin(mCurrentGroupList), it);
    auto first = QAbstractTableModel::index(row, 0);
    auto last  = QAbstractTableModel::index(row, (int)Columns::LAST);
    emit dataChanged(first, last);
}

void NewsList::setAccountFavorites()
{
    Q_ASSERT(mCurrentAccountId);
    QStringList list;
    for (const auto& group : mCurrentGroupList)
    {
        if (group.flags & Flags::Favorite)
            list << group.name;
    }

    auto* account = g_accounts->findAccount(mCurrentAccountId);
    Q_ASSERT(account);

    account->favorites = list;
}

} // app
