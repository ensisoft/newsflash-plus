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

#define MODEL_IMPL
#define LOGTAG "rss"

#include <newsflash/sdk/eventlog.h>
#include <newsflash/sdk/debug.h>
#include <newsflash/sdk/format.h>

#include <newsflash/warnpush.h>
#  include <QtNetwork/QNetworkRequest>
#  include <QtNetwork/QNetworkReply>
#  include <QtXml/QDomDocument>
#  include <QIODevice>
#  include <QDateTime>
#  include <QUrl>
#  include <QByteArray>
#  include <QBuffer>
#include <newsflash/warnpop.h>

#include "model.h"

namespace rss
{

model::model(sdk::hostapp& host) : host_(host)
{
    DEBUG("rss::model created");

    //settings_.streams = sdk::media::none;
}

model::~model()
{
    DEBUG("rss::model deleted");
}

void model::load_content()
{

}

QAbstractItemModel* model::view() 
{
    return this;
}

QString model::name() const
{
    return "rss";
}

void model::complete(sdk::request* request) 
{
    std::unique_ptr<sdk::request> carcass(request);

    // if (model::rss* rss = dynamic_cast<model::rss*>(request))
    // {
    //     const auto& items = rss->items_;
    //     if (items.empty())
    //         return;

    //     beginInsertRows(QModelIndex(),
    //         items_.size(), items_.size() + rss->items_.size());

    //     std::copy(std::begin(items), std::end(items),
    //         std::back_inserter(items_));

    //     endInsertRows();
    // }
    // else if (model::nzb* nzb = dynamic_cast<model::nzb*>(request))
    // {

    // }
}

QVariant model::data(const QModelIndex& index, int role) const
{
    const auto& item = items_[index.row()];

    if (role == Qt::DisplayRole)
    {
        const auto& now = QDateTime::currentDateTime();

        switch (columns(index.column()))
        {
            case columns::date:
                return sdk::format(sdk::event{item.date, now}); 

            case columns::category:
                return sdk::str(item.type);

            case columns::title:
                return item.title;

            default:
                Q_ASSERT(!"unknown column");
                break;
        }
    }
    else if (role == Qt::DecorationRole)
    {

    }
    return QVariant();
}

QVariant model::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (columns(section))
    {
        case columns::date:
            return "Date";

        case columns::category:
           return "Category";

        case columns::title:
            return "Title";

        default:
           Q_ASSERT(!"unknown column");
           break;
    }

    return QVariant();
}

int model::rowCount(const QModelIndex&) const
{
    return (int)items_.size();
}


int model::columnCount(const QModelIndex&) const
{
    return (int)columns::sentinel;
}

} // rss


MODEL_API sdk::model* create_model(sdk::hostapp* host, const char* klazz)
{
    if (std::strcmp(klazz, "rss"))
        return nullptr;

    try 
    {
        return new rss::model(*host);
    }
    catch (const std::exception& e)
    {
        ERROR(sdk::str("rss model exception _1", e.what()));
    }
    return nullptr;
}

MODEL_API int model_api_version()
{
    return sdk::model::version;
}

MODEL_API void model_lib_version(int* major, int* minor)
{
    *major = 1;
    *minor = 0;
}
