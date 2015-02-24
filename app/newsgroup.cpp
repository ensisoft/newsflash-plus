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
#  include <QFile>
#  include <QFileInfo>
#include <newsflash/warnpop.h>
#include "newsgroup.h"
#include "eventlog.h"
#include "debug.h"
#include "platform.h"

namespace app
{

NewsGroup::NewsGroup()
{
    DEBUG("NewsGroup created");
}

NewsGroup::~NewsGroup()
{
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
            case Columns::BinaryFlag:
            case Columns::NewFlag:
            case Columns::DownloadFlag:
            case Columns::BrokenFlag:
            case Columns::BookmarkFlag:
                return ""; 

            case Columns::Age:     return "Age";
            case Columns::Size:    return "Size";
            case Columns::Author:  return "Author";
            case Columns::Subject: return "Subject";
             case Columns::SENTINEL: break;
        }
    }
    else if (role == Qt::DecorationRole)
    {
        static const QIcon icoBinary(toGrayScale(QPixmap("icons:ico_flag_binary.png")));
        static const QIcon icoNew(toGrayScale(QPixmap("icons:ico_flag_new.png")));
        static const QIcon icoDownload(toGrayScale(QPixmap("icons:ico_flag_download.png")));
        static const QIcon icoBroken(toGrayScale(QPixmap("icons:ico_flag_broken.png")));
        static const QIcon icoBookmark(toGrayScale(QPixmap("icons:ico_flag_bookmark.png")));

        switch ((Columns)section)
        {
            case Columns::BinaryFlag:   return icoBinary;
            case Columns::NewFlag:      return icoNew;
            case Columns::DownloadFlag: return icoDownload;
            case Columns::BrokenFlag:   return icoBroken;
            case Columns::BookmarkFlag: return icoBookmark;
            default: break;
        }
    }
    else if (role == Qt::ToolTipRole)
    {
        switch ((Columns)section)
        {
            case Columns::BinaryFlag:   return "Sort by binary status";
            case Columns::NewFlag:      return "Sort by new status";
            case Columns::DownloadFlag: return "Sort by download status";
            case Columns::BrokenFlag:   return "Sort by broken status";
            case Columns::BookmarkFlag: return "Sort by bookmark status";
            case Columns::Age:          return "Sort by age";
            case Columns::Size:         return "Sort by size";
            case Columns::Author:       return "Sort by author";
            case Columns::Subject:      return "Sort by subject";
            case Columns::SENTINEL: break;
        }
    }
    return QVariant();
}

QVariant NewsGroup::data(const QModelIndex& index, int role) const
{
    return QVariant();
}

int NewsGroup::rowCount(const QModelIndex&) const 
{
    return 0;
}

int NewsGroup::columnCount(const QModelIndex&) const 
{
    return (int)Columns::SENTINEL;
}

} // app
