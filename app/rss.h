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

#pragma once

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QString>
#  include <QObject>
#  include <QAbstractTableModel>
#  include <QDateTime>
#include <newsflash/warnpop.h>

#include <memory>
#include <vector>
#include <map>
#include <functional>
#include "mainmodel.h"
#include "media.h"

namespace app
{
    class netreq;
    class mainapp;

    class rss : public mainmodel, public QAbstractTableModel
    {
    public:
        rss(mainapp& app);
       ~rss();
        
        std::function<void()> on_ready;

        virtual void load(const datastore& store) override;

        virtual void save(datastore& store) const override;

        virtual void complete(std::unique_ptr<netreq> request) override;

        virtual void clear() override;

        virtual QAbstractItemModel* view() override;

        // QAbstractTableModel
        virtual QVariant data(const QModelIndex& index, int role) const override;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        virtual void sort(int column, Qt::SortOrder order) override;
        virtual int rowCount(const QModelIndex&) const  override;
        virtual int columnCount(const QModelIndex&) const override;

        bool refresh(media m);

        bool save_nzb(int row, const QString& folder);

        void set_params(const QString& site, const QVariantMap& values);
        
        void enable(const QString& site, bool val);        

        void stop();

        void download(int row, const QString& folder);

    private:
        struct item {
            media type;
            QString title;
            QString id;
            QString nzbLink;
            QDateTime pubdate;
            quint64 size;
            bool password;
        };

    private:
        class feed;
        class womble;
        class nzbs;
        class refresh;
        class savenzb;
        class download;

    private:
        enum class columns {
            date, category, size, title, sentinel
        };                

    private:
        std::vector<std::unique_ptr<feed>> feeds_;
        std::vector<item> items_;            
        std::size_t pending_;                        
        std::map<QString, bool> enabled_;
        mainapp& app_;
    };

} // rss
