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

#define LOGTAG "nzb"

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QtGui/QIcon>
#  include <QFile>
#  include <QBuffer>
#  include <QAbstractListModel>
#include "newsflash/warnpop.h"

#include "engine/nntp.h"
#include "nzbfile.h"
#include "nzbthread.h"
#include "debug.h"
#include "format.h"
#include "eventlog.h"
#include "filetype.h"
#include "engine.h"
#include "types.h"
#include "download.h"

namespace app
{

NZBFile::NZBFile(MediaType mediaType) : mMediatype(mediaType)
{
    DEBUG("NZBFile created");
}

NZBFile::NZBFile()
{
    DEBUG("NZBFile created");
}

NZBFile::~NZBFile()
{
    if (mThread.get())
    {
        DEBUG("Waiting nzb parser thread....");
        mThread->wait();
        mThread.reset();
    }

    DEBUG("NZBFile destroyed");
}

bool NZBFile::load(const QString& file)
{
    std::unique_ptr<QFile> io(new QFile(file));
    if (!io->open(QIODevice::ReadOnly))
    {
        ERROR("Failed to open file %1, %2", file, io->error());
        return false;
    }

    if (mThread.get())
    {
        DEBUG("Waiting existing parsing thread...");
        mThread->wait();
        mThread.reset();
    }
    mSourceDesc = file;
    mThread.reset(new NZBThread(std::move(io)));

    QObject::connect(mThread.get(), SIGNAL(complete()),
        this, SLOT(parseComplete()), Qt::QueuedConnection);

    mThread->start();
    return true;
}

bool NZBFile::load(const QByteArray& bytes, const QString& desc)
{
    if (mThread.get())
    {
        DEBUG("Waiting existing parsing thread...");
        mThread->wait();
        mThread.reset();
    }
    mSourceBuffer = bytes;
    mSourceDesc   = desc;

    std::unique_ptr<QBuffer> io(new QBuffer);
    io->setBuffer(&mSourceBuffer);

    mThread.reset(new NZBThread(std::move(io)));

    QObject::connect(mThread.get(), SIGNAL(complete()),
        this, SLOT(parseComplete()), Qt::QueuedConnection);

    mThread->start();
    return true;
}

void NZBFile::downloadSel(const QModelIndexList& list, quint32 account, const QString& folder, const QString& desc)
{
    std::vector<NZBContent> selected;
    selected.reserve(list.size());

    for (const auto& i : list)
        selected.push_back(mData[i.row()]);

    Download download;
    download.type     = findMediaType();
    download.source   = MediaSource::File;
    download.account  = account;
    download.basepath = folder;
    download.folder   = desc;
    download.desc     = desc;
    g_engine->downloadNzbContents(download, std::move(selected));
}

void NZBFile::downloadAll(quint32 account, const QString& folder, const QString& desc)
{
    std::vector<NZBContent> everything = mData;

    Download download;
    download.type     = findMediaType();
    download.source   = MediaSource::File;
    download.account  = account;
    download.basepath = folder;
    download.folder   = desc;
    download.desc     = desc;
    g_engine->downloadNzbContents(download, std::move(everything));
}

quint64 NZBFile::sumDataSizes(const QModelIndexList& list) const
{
    quint64 bytes = 0;
    for (const auto& i : list)
    {
        const auto row = i.row();
        const auto& item = mData[row];
        bytes += item.bytes;
    }
    return bytes;
}

quint64 NZBFile::sumDataSizes() const
{
    quint64 bytes = 0;
    for (const auto& item : mData)
    {
        bytes += item.bytes;
    }
    return bytes;
}

void NZBFile::setShowFilenamesOnly(bool on_off)
{
    if (on_off == mShowFilenameOnly)
        return;
    mShowFilenameOnly = on_off;

    auto first = QAbstractTableModel::index(0, 0);
    auto last  = QAbstractTableModel::index(mData.size(), 3);
    emit dataChanged(first, last);
}

const NZBContent& NZBFile::getItem(std::size_t index) const
{
    BOUNDSCHECK(mData, index);
    return mData[index];
}

MediaType NZBFile::findMediaType() const
{
    if (mMediatype != MediaType::SENTINEL)
        return mMediatype;

    // we're going to speculate for the media type the
    // based on the items that we have.

    for (const auto& item : mData)
    {
        for (const auto& newsgroup : item.groups)
        {
            const auto type = app::findMediaType(fromUtf8(newsgroup));
            if (type != MediaType::Other)
            {
                if (mMediatype == MediaType::SENTINEL)
                    mMediatype = type;
                else if (mMediatype != type)
                {
                    mMediatype = MediaType::Other;
                    return mMediatype;
                }
            }
        }
    }
    if (mMediatype == MediaType::SENTINEL)
        mMediatype = MediaType::Other;

    return mMediatype;
}

int NZBFile::rowCount(const QModelIndex&) const
{
    return (int)mData.size();
}

int NZBFile::columnCount(const QModelIndex&) const
{
    return (int)Columns::LAST;
}

void NZBFile::sort(int column, Qt::SortOrder order)
{
    emit layoutAboutToBeChanged();
    if (column == (int)Columns::Type)
    {
        std::sort(std::begin(mData), std::end(mData),
            [&](const NZBContent& lhs, const NZBContent& rhs) {
                if (order == Qt::AscendingOrder)
                    return lhs.type < rhs.type;
                return lhs.type > rhs.type;
            });
    }
    else if (column == (int)Columns::Size)
    {
        std::sort(std::begin(mData), std::end(mData),
            [=](const NZBContent& lhs, const NZBContent& rhs) {
                if (order == Qt::AscendingOrder)
                    return lhs.bytes < rhs.bytes;
                return lhs.bytes > rhs.bytes;
            });
    }
    else if (column == (int)Columns::File)
    {
        std::sort(std::begin(mData), std::end(mData),
            [&](const NZBContent& lhs, const NZBContent& rhs) {
                if (order == Qt::AscendingOrder)
                    return lhs.subject < rhs.subject;
                return lhs.subject > rhs.subject;
            });
    }
    emit layoutChanged();
}

QVariant NZBFile::data(const QModelIndex& index, int role) const
{
    const auto row = index.row();
    const auto col = index.column();
    const auto& item = mData[row];

    if (role == Qt::DisplayRole)
    {
        switch ((Columns)col)
        {
            case Columns::Type: return toString(item.type);
            case Columns::Size: return toString(app::size{item.bytes});
            case Columns::File:
                if (mShowFilenameOnly)
                {
                    const auto& utf8 = toUtf8(item.subject);
                    const auto& name = nntp::find_filename(utf8);
                    if (name.size() > 5)
                        return fromUtf8(name);
                }
                return item.subject;
            case Columns::LAST: Q_ASSERT(0);
        }
    }
    else if (role == Qt::DecorationRole)
    {
        if ((Columns)col == Columns::Type)
            return item.icon;
    }
    return QVariant();
}

QVariant NZBFile::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch ((Columns)section)
    {
        case Columns::Type: return "Type";
        case Columns::Size: return "Size";
        case Columns::File: return "File";
        case Columns::LAST: Q_ASSERT(0);
    }
    return {};
}

void NZBFile::parseComplete()
{
    DEBUG("Parse_complete %1", mSourceDesc);

    const auto result = mThread->result(mData);
    switch (result)
    {
        case NZBError::None:
            DEBUG("NZB parse succesful!");
            break;

        case NZBError::XML:
            ERROR("XML error while parsing NZB file %1", mSourceDesc);
            break;

        case NZBError::NZB:
            ERROR("Error in NZB file content %1", mSourceDesc);
            break;

        case NZBError::Io:
            ERROR("I/O error while parsing NZB file %1",mSourceDesc);
            break;

        case NZBError::Other:
            ERROR("Unknown error while parsing NZB file %1", mSourceDesc);
            break;
    }
    for (auto& item : mData)
    {
        const auto utf8 = app::toUtf8(item.subject);
        auto name = nntp::find_filename(utf8);
        if (name.size() < 5)
            name = utf8;
        if (!name.empty())
        {
            item.type = findFileType(app::fromUtf8(name));
            item.icon = findFileIcon(item.type);
        }
    }

    reset();

    on_ready(result == NZBError::None);
}


} // app
