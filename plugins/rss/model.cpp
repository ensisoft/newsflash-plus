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

#define PLUGIN_IMPL

#define LOGTAG "rss"

#include <newsflash/sdk/eventlog.h>
#include <newsflash/sdk/debug.h>
#include <newsflash/sdk/format.h>
#include <newsflash/sdk/dist.h>

#include <newsflash/warnpush.h>
#  include <QtNetwork/QNetworkRequest>
#  include <QtNetwork/QNetworkReply>
#  include <QtXml/QDomDocument>
#  include <QIODevice>
#  include <QDateTime>
#  include <QUrl>
#  include <QByteArray>
#  include <QBuffer>
#  include <QLibrary>
#  include <QDir>
#include <newsflash/warnpop.h>

#include "model.h"

using sdk::str;

namespace {

class get_rss : public sdk::request 
{
public:
    get_rss(QUrl url) : url_(std::move(url))
    {}
    virtual void prepare(QNetworkRequest& request) override
    {
        request.setUrl(url_);
    }
    virtual void receive(QNetworkReply& reply) override
    {
        if (reply.error() != QNetworkReply::NoError)
            return;

        QByteArray bytes = reply.readAll();
        QBuffer io(&bytes);
        feed_->parse(io, items_);
    }
    void append(std::vector<sdk::rssfeed::item>& vec) const
    {
        std::copy(std::begin(items_), std::end(items_),
            std::back_inserter(vec));
    }
    std::size_t size() const 
    {
        return items_.size();
    }
private:
    QUrl url_;
private:
    sdk::rssfeed* feed_;
private:
    std::vector<sdk::rssfeed::item> items_;

};

class get_nzb : public sdk::request
{
public:
    get_nzb(QUrl url) : url_(std::move(url))
    {}

    virtual void prepare(QNetworkRequest& request) override
    {}

    virtual void receive(QNetworkReply& reply) override
    {
        if (reply.error() != QNetworkReply::NoError)
            return;

        data_ = reply.readAll();
    }

private:
    QUrl url_;
private:
    QByteArray data_;
};


} // namespace

namespace rss
{

model::model(sdk::hostapp& host) : host_(host)
{
    DEBUG(str("rss::model _1 created", (const void*)this));

    INFO("RSS plugin 1.0 (c) 2014 Sami Vaisanen");


    const QDir dir(sdk::dist::path("plugins/rss"));

    for (const auto& name : dir.entryList())
    {
        const auto file = dir.absoluteFilePath(name);
        if (!QLibrary::isLibrary(file))
            continue;

        QLibrary lib(file);

        DEBUG(str("Loading library _1", lib));

        if (!lib.load())
        {
            ERROR(lib.errorString());
        }
        auto create = (sdk::fp_create_rssfeed)(lib.resolve("create_rssfeed"));
        if (create == nullptr)
        {
            ERROR(str("Failed to load _1, no entry point found", lib));
            continue;
        }
        std::unique_ptr<sdk::rssfeed> feed(create(sdk::rssfeed::version));
        if (!feed)
            continue;

        INFO(str("Available RSS provider _1", feed->site()));

        feeds_.push_back(std::move(feed));
    }
}

model::~model()
{
    DEBUG(str("rss::model _1 deleted", (const void*)this));;
}

void model::refresh(sdk::category cat)
{
    items_.clear();

    QAbstractTableModel::reset();

    std::vector<QUrl> urls;

    for (const auto& feed : feeds_)
    {
        feed->prepare(cat, urls);
    }

    for (const auto& url : urls)
    {
        std::unique_ptr<get_rss> get(new get_rss(url));

        host_.submit(this, get.get());

        //grabs_.push_back(std::move(get));
    }
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

    if (get_rss* rss = dynamic_cast<get_rss*>(request))
    {
        const auto count = rss->size();
        beginInsertRows(QModelIndex(),
            items_.size(), items_.size() + count);
        rss->append(items_);
        endInsertRows();
    }
    else if (get_nzb* nzb = dynamic_cast<get_nzb*>(request))
    {

    }
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
                return sdk::format(sdk::event{item.pubdate, now}); 

            case columns::category:
                return sdk::str(item.cat);

            case columns::title:
                return item.title;

            case columns::size:
                if (item.size == 0)
                    return "N/A";
                return sdk::str(sdk::size { item.size });

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

        case columns::size:
           return "Size";

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


PLUGIN_IMPL sdk::model* create_model(sdk::hostapp* host, const char* klazz, int version)
{
    if (std::strcmp(klazz, "rss"))
        return nullptr;

    if (version != sdk::model::version)
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
