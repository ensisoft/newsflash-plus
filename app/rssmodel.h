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

#pragma once

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QIcon>
#  include <QVariant>
#include <newsflash/warnpop.h>
#include "media.h"
#include "types.h"
#include "format.h"
#include "platform.h"
#include "utility.h"


namespace app
{
    class RSSModel 
    {
    public:
        enum class Columns {
            Date, Type, Size, Locked, Title, LAST
        };

        QVariant cellData(const MediaItem& item, Columns col) const 
        {
            switch (col)
            {
                case Columns::Date:   return toString(app::event { item.pubdate });
                case Columns::Type:   return toString(item.type);
                case Columns::Title:  return item.title;
                case Columns::Locked: return "";

                case Columns::Size: 
                    if (item.size == 0)
                        return "n/a";
                    return toString(app::size { item.size });

                case Columns::LAST: Q_ASSERT(false);
            }
            return {};
        }
        QIcon cellIcon(const MediaItem& item, Columns col) const 
        {
            if (col == Columns::Date)
                return QIcon("icons:ico_rss.png");
            if (col == Columns::Locked && item.password)
                return QIcon("icons:ico_password.png");

            return {};
        }
        QString name(Columns col) const 
        {
            switch (col)
            {
                case Columns::Date:   return "Date";
                case Columns::Type:   return "Category";
                case Columns::Size:   return "Size";
                case Columns::Title:  return "Title";
                case Columns::Locked: return "";
                case Columns::LAST: Q_ASSERT(false);
            }
            return {};
        }
        QIcon icon(Columns col) const 
        {
            static const auto ico(toGrayScale("icons:ico_password.png"));
            if (col == Columns::Locked)
                return ico;

            return {};
        }
        QVariant size(Columns col) const 
        {
            if (col == Columns::Locked)
                return QSize {32, 0};

            return {};
        }

        void sort(std::vector<MediaItem>& items, Columns col, Qt::SortOrder order)
        {
            switch (col)
            {
                case Columns::Date: 
                    app::sort(items, order, &MediaItem::pubdate);
                    break;
                case Columns::Type:
                    app::sort(items, order, &MediaItem::type);
                    break;
                case Columns::Size:
                    app::sort(items, order, &MediaItem::size);
                    break;
                case Columns::Title:
                    app::sort(items, order, &MediaItem::title);
                    break;
                case Columns::Locked:
                    app::sort(items, order, &MediaItem::password);
                    break;
                case Columns::LAST: Q_ASSERT(0);
            }
        }

    private:
    };
} // app