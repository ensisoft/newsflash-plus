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

#include <newsflash/sdk/rssmodel.h>
#include <newsflash/sdk/hostapp.h>
#include <newsflash/sdk/request.h>
#include <newsflash/sdk/rssfeed.h>
#include <newsflash/sdk/datastore.h>

#include <newsflash/warnpush.h>
#  include <QAbstractItemModel>
#  include <QString>
#include <newsflash/warnpop.h>

#include <memory>
#include <vector>
#include <map>

namespace rss 
{
    class model : public QAbstractTableModel, public sdk::rssmodel
    {
    public:
        model(sdk::hostapp& host);
       ~model();
        
        virtual void load(const sdk::datastore& store) override;

        virtual void save(sdk::datastore& store) const override;

        // sdk::model
        virtual void complete(sdk::request* request) override;

        // sdk::model
        virtual void clear() override;

        // sdk::model
        virtual QString name() const override;

        // sdk::rssmodel
        virtual bool refresh(sdk::category cat) override;

        virtual QList<QString> sites() const override;

        virtual bool get_params(const QString& site, QVariantMap& values) const override;

        virtual bool set_params(const QString& site, const QVariantMap& values) override;

        virtual void enable(const QString& site, bool val) override;

        // QAbstractItemModel
        virtual QAbstractItemModel* view() override;

        // AbstractItemModel
        virtual QVariant data(const QModelIndex&, int role) const override;

        // AbstractItemModel
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

        virtual void sort(int column, Qt::SortOrder order) override;
        // AbstractItemModel
        virtual int rowCount(const QModelIndex&) const override;

        // AbstractItemModel
        virtual int columnCount(const QModelIndex&) const override;
    private:
        enum class columns {
            date, category, size, title, sentinel
        };        

    private:
        std::vector<std::unique_ptr<sdk::rssfeed>> feeds_;
        using item = sdk::rssfeed::item;        
        std::vector<item> items_;
        std::size_t pending_;                        
        std::map<QString, bool> enabled_;
    private:
        sdk::hostapp& host_;
    private:
        columns sortcol_;
        Qt::SortOrder sortorder_;


    };

} // rss