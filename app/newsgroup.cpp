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
#  include <QtGui/QIcon>
#  include <QtGui/QFont>
#  include <QtGui/QBrush>
#  include <QFile>
#  include <QFileInfo>
#  include <QDir>
#  include <QCoreApplication>
#include <newsflash/warnpop.h>
#include <newsflash/engine/nntp.h>
#include "newsgroup.h"
#include "eventlog.h"
#include "debug.h"
#include "platform.h"
#include "engine.h"
#include "utility.h"
#include "format.h"
#include "types.h"
#include "fileinfo.h"
#include "filetype.h"

namespace app
{

QString toString(const str::string_view& str)
{
    return QString::fromLatin1(str.c_str(), str.size());
}

NewsGroup::NewsGroup() : task_(0)
{
    DEBUG("NewsGroup created");

    QObject::connect(g_engine, SIGNAL(newHeaderDataAvailable(const QString&)),
        this, SLOT(newHeaderDataAvailable(const QString&)));
    QObject::connect(g_engine, SIGNAL(newHeaderInfoAvailable(const QString&, quint64, quint64)),
        this, SLOT(newHeaderInfoAvailable(const QString&, quint64, quint64)));
    QObject::connect(g_engine, SIGNAL(updateCompleted(const app::HeaderInfo&)),
        this, SLOT(updateCompleted(const app::HeaderInfo&)));

    // loader callback
    index_.on_load = [this] (std::size_t key, std::size_t idx) {
        auto& catalog = catalogs_[key];
        return catalog.db.load(catalog::index_t{idx});
    };

}

NewsGroup::~NewsGroup()
{
    if (task_)
    {
        g_engine->killAction(task_);
    }

    DEBUG("Newsgroup deleted");
}

QVariant NewsGroup::headerData(int section, Qt::Orientation orientation, int role) const 
{
    if (orientation != Qt::Horizontal)
        return QVariant();

    if (role == Qt::DisplayRole)
    {

        switch ((Columns)section)
        {
            // fallthrough intentional for these cases.
            case Columns::BrokenFlag:          
            case Columns::DownloadFlag:
            case Columns::BookmarkFlag:
                return ""; 

            case Columns::Type:    return "File";
            case Columns::Age:     return "Age";
            case Columns::Size:    return "Size";
            case Columns::Subject: return "Subject";
            case Columns::LAST:    break;
        }
    }
    else if (role == Qt::DecorationRole)
    {
        static const QIcon icoBinary(toGrayScale("icons:ico_flag_binary.png"));
        static const QIcon icoDownload(toGrayScale("icons:ico_flag_download.png"));
        static const QIcon icoBroken(toGrayScale("icons:ico_flag_broken.png"));
        static const QIcon icoBookmark(toGrayScale("icons:ico_flag_bookmark.png"));

        switch ((Columns)section)
        {
            case Columns::BrokenFlag:   return icoBroken;            
            case Columns::DownloadFlag: return icoDownload;
            case Columns::BookmarkFlag: return icoBookmark;
            default: break;
        }
    }
    else if (role == Qt::ToolTipRole)
    {
        switch ((Columns)section)
        {
            case Columns::Type:         return "Sort by file type";
            case Columns::BrokenFlag:   return "Sort by broken status";            
            case Columns::DownloadFlag: return "Sort by download status";
            case Columns::BookmarkFlag: return "Sort by bookmark status";
            case Columns::Age:          return "Sort by age";
            case Columns::Size:         return "Sort by size";
            case Columns::Subject:      return "Sort by subject";
            case Columns::LAST: break;
        }
    }
    return QVariant();
}

QVariant NewsGroup::data(const QModelIndex& index, int role) const
{
    const auto row   = (std::size_t)index.row();
    const auto col   = (Columns)index.column();
    const auto& item = index_[row];    

    using flags = article::flags;
    using type  = newsflash::filetype;

    if (role == Qt::DisplayRole)
    {
        switch (col)
        {
            // fall-through intended.            
            case Columns::BrokenFlag:
            case Columns::DownloadFlag:
            case Columns::BookmarkFlag:
                return {}; 

            case Columns::Type:
                switch (item.type()) 
                {
                    case type::none:     return "n/a";
                    case type::audio:    return toString(FileType::Audio);
                    case type::video:    return toString(FileType::Video);
                    case type::image:    return toString(FileType::Image);
                    case type::text:     return toString(FileType::Text);                                                            
                    case type::archive:  return toString(FileType::Archive);
                    case type::parity:   return toString(FileType::Parity);
                    case type::document: return toString(FileType::Document);
                    case type::other:    return toString(FileType::Other);
                    default: Q_ASSERT(!"unknown file type");
                }
                break;

            case Columns::Age:    
                if (item.pubdate() == 0) 
                    return "???";
                return toString(age { QDateTime::fromTime_t(item.pubdate()) });

            case Columns::Size:    
                return toString(size { item.bytes() });

            case Columns::Subject: 
                return toString(item.subject());

            case Columns::LAST: Q_ASSERT(false);
        }
    }
    else if (role == Qt::DecorationRole)
    {
        switch (col)
        {
            case Columns::BrokenFlag:
                if (item.test(flags::broken))
                    return QIcon("icons:ico_flag_broken.png");
                break;

            case Columns::DownloadFlag:
                if (item.test(flags::downloaded))
                    return QIcon("icons:ico_flag_downloaded.png");
                break;

            case Columns::BookmarkFlag:
                if (item.test(flags::bookmarked))
                    return QIcon("icons:ico_flag_bookmark.png");
                break;

            case Columns::Type:
                switch (item.type())
                {
                    case type::none:     return findFileIcon(FileType::Text);
                    case type::audio:    return findFileIcon(FileType::Audio);
                    case type::video:    return findFileIcon(FileType::Video);
                    case type::image:    return findFileIcon(FileType::Image);
                    case type::text:     return findFileIcon(FileType::Text);                                                            
                    case type::archive:  return findFileIcon(FileType::Archive);
                    case type::parity:   return findFileIcon(FileType::Parity);
                    case type::document: return findFileIcon(FileType::Document);
                    case type::other:    return findFileIcon(FileType::Other);   
                    default: Q_ASSERT(!"unknown file type");
                }
                break;


            case Columns::Age:
            case Columns::Size:
            case Columns::Subject:
                break;
            case Columns::LAST: Q_ASSERT(0);
        }
    }
    return {};
}

int NewsGroup::rowCount(const QModelIndex&) const 
{
    return (int)index_.size();
}

int NewsGroup::columnCount(const QModelIndex&) const 
{
    return (int)Columns::LAST;
}

void NewsGroup::sort(int column, Qt::SortOrder order)
{
    DEBUG("Sorting to %1 order by column %2", order, column);

    emit layoutAboutToBeChanged();

    const auto o = (order == Qt::AscendingOrder) ? 
        index::order::ascending :
        index::order::descending;

    DEBUG("Index has %1 articles", index_.size());

    switch ((Columns)column)
    {
        case Columns::Type: 
            index_.sort(index::sorting::sort_by_type, o);
            break;

        case Columns::BrokenFlag: 
            index_.sort(index::sorting::sort_by_broken, o);
            break;        

        case Columns::DownloadFlag: 
            index_.sort(index::sorting::sort_by_downloaded, o);
            break;

        case Columns::BookmarkFlag: 
            index_.sort(index::sorting::sort_by_bookmarked, o);
            break;

        case Columns::Age: 
            index_.sort(index::sorting::sort_by_age, o);
            break;

        case Columns::Subject:
            index_.sort(index::sorting::sort_by_subject, o);
            break;

        case Columns::Size:
            index_.sort(index::sorting::sort_by_size, o);
            break;

        case Columns::LAST: Q_ASSERT(0); break;
    }

    emit layoutChanged();
}

bool NewsGroup::load(quint32 blockIndex, QString path, QString name, quint32& numBlocks)
{
    DEBUG("Load data for %1 from %2", name, path);

    // data is in .vol files with the file number indicating data ordering.
    // i.e. the bigger the index in the datafile name the newer the data.
    QDir dir;
    dir.setPath(joinPath(path, name));
    dir.setSorting(QDir::Name | QDir::Reversed);
    dir.setNameFilters(QStringList("vol*.dat"));
    QStringList files = dir.entryList();
    numBlocks = files.size();

    if (files.isEmpty())
        return false;

    if (blockIndex >= files.size())
        return false;

    const auto p = joinPath(path, name);
    const auto f = joinPath(p, files[blockIndex]);
    DEBUG("Loading headers from %1", f);

    loadMoreData(narrow(f), true);
    return true;
}

void NewsGroup::refresh(quint32 account, QString path, QString name)
{
    Q_ASSERT(task_ == 0);        

    DEBUG("Refresh data for %1 in %2", name, path);
    path_ = path;
    name_ = name;
    task_ = g_engine->retrieveHeaders(account, path, name);
}

void NewsGroup::stop()
{
    Q_ASSERT(task_ != 0);

    g_engine->killAction(task_);

    task_ = 0;
}

std::size_t NewsGroup::numItems() const 
{
    return index_.size();
}

void NewsGroup::newHeaderDataAvailable(const QString& file)
{
    DEBUG("New headers available in %1", file);

    // see if this data file is of interest to us. if it isn't just ignore the signal
    if (file.indexOf(joinPath(path_, name_)) != 0)
        return;

    loadMoreData(narrow(file), false);
}

void NewsGroup::newHeaderInfoAvailable(const QString& group, quint64 numLocal, quint64 numRemote)
{
    // nothing to do for now
}

void NewsGroup::updateCompleted(const app::HeaderInfo& info)
{
    if (info.groupName != name_ || info.groupPath != path_)
        return;

    task_ = 0;
}

void NewsGroup::loadMoreData(const std::string& file, bool guiLoad)
{
    QAbstractTableModel::beginResetModel();

    auto it = std::find_if(std::begin(catalogs_), std::end(catalogs_),
        [&](const Catalog& cat) {
            return cat.db.filename() == file;
        });

    if (it == std::end(catalogs_))
    {
        Catalog next;
        next.prevSize   = 0;
        next.prevOffset = 0;
        it = catalogs_.insert(catalogs_.end(), next);
    }
    auto& cat = *it;
    auto& db  = (*it).db;
    db.open(file);
    if (db.article_count() == cat.prevSize)
        return;

    DEBUG("%1 is at offset %2", file, cat.prevOffset);
    DEBUG("Index has %1 articles. Loading more...", index_.size());

    const auto index = std::distance(std::begin(catalogs_), it);

    std::size_t curItem  = 0;
    std::size_t numItems = db.article_count();

    // load all the articles, starting at the latest offset that we know off.
    auto beg = db.begin(cat.prevOffset);
    auto end = db.end();
    for (; beg != end; ++beg)
    {
        const auto& article  = *beg;

        index_.insert(article, index, article.index());        

        if (guiLoad)
        {
            onLoadProgress(curItem, numItems);
            ++curItem;
        }
    }

    DEBUG("Load done. Index now has %1 articles", index_.size());

    // save the database and the current offset
    // when new data is added to the same database
    // we reload the db and then use the current latest
    // index to start loading the objects.
    cat.prevOffset = beg.offset();
    cat.prevSize   = db.article_count();
    DEBUG("%1 is at new offst %2", file, cat.prevOffset);

    QAbstractTableModel::reset();    
    QAbstractTableModel::endResetModel();    
}

} // app
