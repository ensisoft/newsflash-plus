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

#define LOGTAG "rss"

#include <newsflash/warnpush.h>
#  include <QtNetwork/QNetworkRequest>
#  include <QtNetwork/QNetworkReply>
#  include <QtGui/QIcon>
#  include <QtXml/QDomDocument>
#  include <QIODevice>
#  include <QDateTime>
#  include <QUrl>
#  include <QByteArray>
#  include <QBuffer>
#  include <QDir>
#include <newsflash/warnpop.h>

#include "rss.h"
#include "rssdate.h"
#include "eventlog.h"
#include "debug.h"
#include "format.h"
#include "netreq.h"
#include "mainapp.h"
#include "nzbparse.h"

namespace app
{


class rss::feed
{
public:
    virtual ~feed() = default;

    virtual void set_params(const QVariantMap& params) = 0;

    virtual bool parse(QIODevice& io, std::vector<rss::item>& rss) const = 0;

    virtual void prepare(media type, std::vector<QUrl>& urls) const = 0;

    virtual QString name() const = 0;
protected:
private:
};

class rss::womble : public rss::feed
{
public:
    virtual void set_params(const QVariantMap& params)
    {}

    virtual bool parse(QIODevice& io, std::vector<rss::item>& rss) const override
    {
        QDomDocument dom;
        QString error_string;
        int error_line;
        int error_column;
        if (!dom.setContent(&io, false, &error_string, &error_line, &error_column))
        {
            ERROR(str("Error parsing RSS response '_1'", error_string));
            return false;
        }
        const auto& root  = dom.firstChildElement();
        const auto& items = root.elementsByTagName("item");
        for (int i=0; i<items.size(); ++i)
        {
            const auto& elem = items.at(i).toElement();

            rss::item item {};
            item.title    = elem.firstChildElement("title").text();
            item.id       = elem.firstChildElement("link").text();
            item.pubdate  = parse_date(elem.firstChildElement("pubDate").text());
            item.password = false;
            item.nzbLink  = item.id;
            item.size     = 0; // not known
            rss.push_back(std::move(item));
        }
        return true;        
    }
    virtual void prepare(media type, std::vector<QUrl>& urls) const override
    {
        struct feed {
            media type;
            const char* arg;
        } feeds[] = {
            {media::console_nds, "nds"},
            {media::console_psp, "psp"},
            {media::console_wii, "wii"},
            {media::console_xbox, "xbox"},
            {media::console_xbox360, "xbox360"},
            {media::console_ps3, "ps3"},
            {media::console_ps2, "ps2"},
            {media::movies_sd, "divx"},
            {media::movies_sd, "xvid"},
            {media::movies_sd, "dvd-pal"},
            {media::movies_hd, "bluray"}, // better than x264
            {media::audio_mp3, "mp3"},
            {media::audio_video, "mvids"}, // mvid and mv are same as mvids
            {media::apps_pc, "0-day"},
            {media::apps_pc, "0day"},
            {media::apps_pc, "0day-"},
            {media::apps_iso, "iso"},
            {media::tv_sd, "tv-xvid"},
            {media::tv_sd, "tv-dvdrip"},
            {media::tv_hd, "tv-x264"},
            {media::xxx_dvd, "xxx"},
            {media::ebook, "ebook"}        
        };

        const QString site("http://newshost.co.za/rss/?sec=%1&fr=false");

        for (const auto& feed : feeds)
        {
            if (feed.type == type)
            {
                QUrl url(site.arg(feed.arg));
                urls.push_back(url);
            }
        }        
    }
    virtual QString name() const override
    {
        return "womble";
    }
private:
    QDateTime parse_date(const QString& str) const
    {
        // this appears to be womble's format.. is this according to some RFC?
        // certainly not according to the format in RSS specification
        // 7/8/2014 1:53:02 AM
        auto ret = QDateTime::fromString(str, "M/d/yyyy h:m:s AP");

        // womble is .za which is south-africa. UTC+2
        // todo: daylight saving time??
        const auto time = ret.time();
        time.addSecs(- 60 * 60 * 2);
        ret.setTime(time);
        return ret;        
    }
};

class rss::nzbs : public feed
{
public:
    virtual void set_params(const QVariantMap& params)
    {
        userid_   = params["userid"].toString();
        apikey_   = params["apikey"].toString();
        feedsize_ = params["feedsize"].toInt();
    }
    bool parse(QIODevice& io, std::vector<item>& rss) const
    {
        QDomDocument dom;
        QString error_string;
        int error_line   = 0;
        int error_column = 0;
        if (!dom.setContent(&io, false, &error_string, &error_line, &error_column))
        {
            ERROR(str("Error parsing RSS response '_1'", error_string));
            return false;
        }
        const auto& root  = dom.firstChildElement();
        const auto& items = root.elementsByTagName("item");
        for (int i=0; i<items.size(); ++i)
        {
            const auto& elem = items.at(i).toElement();

            rss::item item {};
            item.title   = elem.firstChildElement("title").text();
            item.id      = elem.firstChildElement("guid").text();
            item.nzbLink = elem.firstChildElement("link").text();
            item.pubdate = parse_rss_date(elem.firstChildElement("pubDate").text());

            const auto& attrs = elem.elementsByTagName("newznab:attr");
            for (int x=0; x<attrs.size(); ++x)
            {
                const auto& attr = attrs.at(x).toElement();
                const auto& name = attr.attribute("name");
                if (name == "size")
                    item.size = attr.attribute("value").toLongLong();
                else if (name == "password")
                    item.password = attr.attribute("value").toInt();
            }

            // looks like there's a problem with the RSS feed from nzbs.org. The link contains a link to a
            // HTML info page about the the NZB not an actual link to get the NZB. adding &dl=1 to the 
            // query params doesn't change this.
            // we have a simple hack here to change the feed link to get link
            // http://nzbs.org/index.php?action=view&nzbid=334012
            // http://nzbs.org/index.php?action=getnzb&nzbid=334012
            if (item.nzbLink.contains("action=view&"))
                item.nzbLink.replace("action=view&", "action=getnzb&");

            rss.push_back(std::move(item));
        }
        return true;        
    }
    void prepare(media type, std::vector<QUrl>& urls) const
    {
        struct feed {
            media type;
            const char* arg;
        } feeds[] = {
            {media::console_nds, "1010"},
            {media::console_psp, "1020"},
            {media::console_wii, "1030"},
            {media::console_xbox, "1040"},
            {media::console_xbox360, "1050"},
            {media::console_ps3, "1080"},
            {media::console_ps2, "1090"},
            {media::movies_int, "2060"}, // foreign
            {media::movies_sd, "2010"}, // dvd
            {media::movies_hd, "2040"}, // x264
            {media::movies_hd, "2020"}, // wmv-hd
            {media::audio_mp3, "3010"},
            {media::audio_video, "3020"}, 
            {media::audio_lossless, "3040"}, // flac
            {media::apps_pc, "4010"}, // 0day
            {media::apps_iso, "4020"},
            {media::apps_mac, "4030"},
            {media::apps_android, "4070"},
            {media::apps_ios, "4060"},
            {media::tv_int, "5080"}, // foreign
            {media::tv_sd, "5010"}, // dvd
            {media::tv_sd, "5070"}, // boxsd
            {media::tv_hd, "5040"}, // hd
            {media::tv_hd, "5090"}, // boxhd
            {media::tv_other, "5060"},
            {media::xxx_dvd, "6010"},
            {media::xxx_hd, "6040"}, // x264
            {media::xxx_sd, "6030"}, // xvid
            {media::other, "7010"} // other misc
        };

        Q_ASSERT(feedsize_);
        Q_ASSERT(!userid_.isEmpty());
        Q_ASSERT(!apikey_.isEmpty());

        // nzbs.org supports num=123 parameter for limiting the feed size
        // however since we map multiple rss feeds to a single category
        // it won't work as the user expects.
        const QString site("http://nzbs.org/rss?t=%1&i=%2&r=%3&dl=1");

        for (const auto& feed : feeds)
        {
            if (feed.type == type)
            {
                QUrl url(site.arg(feed.arg)
//                    .arg(feedsize_)
                    .arg(userid_)
                    .arg(apikey_));
                urls.push_back(url);
            }
        }        
    }
    QString name() const
    {
        return "nzbs";
    }
private:
    QString userid_;
    QString apikey_;
    int feedsize_;
};

class rss::refresh : public netreq
{
public:
    refresh(const QUrl& url, media type, feed* feed) : url_(url), type_(type), feed_(feed)
    {}
    virtual void prepare(QNetworkRequest& request) override
    {
        request.setUrl(url_);
    }
    virtual void receive(QNetworkReply& reply) override
    {
        if (reply.error() != QNetworkReply::NoError)
        {
            ERROR("Unable to refresh the RSS content because the network request failed");
            return;
        }

        QByteArray bytes = reply.readAll();
        QBuffer io(&bytes);

        //qDebug() << bytes;

        feed_->parse(io, items_);

        for (auto& item : items_)
        {
            if (item.type == media::none)
                item.type = type_;

            item.pubdate = item.pubdate.toLocalTime();
        }

        INFO(str("RSS feed from _1 complete", url_));
    }
    const
    std::vector<item>& items() const {
        return items_;
    }

private:
    const QUrl url_;
    const media type_;
    const feed* feed_;
private:
    std::vector<item> items_;
};

class rss::savenzb : public netreq
{
public:
    savenzb(QString link, QString file) : link_(std::move(link)), file_(std::move(file))
    {}
   ~savenzb()
    {}

    virtual void prepare(QNetworkRequest& request) override
    {
        request.setUrl(QUrl(link_));
    }

    virtual void receive(QNetworkReply& reply) override
    {
        if (reply.error() != QNetworkReply::NoError)
        {
            ERROR("Unable to save the NZB because the network request failed");
            return;
        }

        QFile file(file_);
        file.open(QIODevice::WriteOnly);
        if (!file.isOpen())
        {
            const auto err = file.error();
            ERROR(str("Failed to write file _1, _2", file, err));
            return;
        }

        QByteArray bytes = reply.readAll();

        file.write(bytes);
        file.flush();
        file.close();

        INFO(str("Saved NZB file _1", file));
    }
private:
    const QString link_;
    const QString file_;
};

class rss::download : public netreq
{
public:
    download(QString link, QString folder) : link_(std::move(link)), folder_(std::move(folder))
    {}

   ~download()
    {}

    virtual void prepare(QNetworkRequest& request) override
    {
        request.setUrl(QUrl(link_));
    }
    virtual void receive(QNetworkReply& reply) override
    {
        if (reply.error() != QNetworkReply::NoError)
        {
            ERROR("Unable to start download because the network request failed");
            return;
        }

        QByteArray bytes = reply.readAll();
        QBuffer io(&bytes);

        std::vector<nzbcontent> content;

        if (parse_nzb(io, content) != nzberror::none)
        {
            ERROR("Parsing NZB content failed");
            return;
        }

        
    }

private:
    const QString link_;
    const QString folder_;
};


rss::rss(mainapp& app) : pending_(0), app_(app)
{
    INFO("RSS http://newshost.co.za");
    INFO("RSS http://nzbs.org");

    feeds_.emplace_back(new womble);
    feeds_.emplace_back(new nzbs);

    DEBUG("rss created");
}

rss::~rss()
{
    DEBUG("rss destroyed");
}

void rss::load(const datastore& store)
{}

void rss::save(datastore& store) const
{}

bool rss::refresh(media type)
{
    auto currently_pending = pending_;

    for (const auto& feed : feeds_)
    {
        const auto& site = feed->name();
        if (!enabled_[site])
            continue;

        std::vector<QUrl> urls;        
        feed->prepare(type, urls);

        for (const auto& url : urls)
        {
            std::unique_ptr<class refresh> action(new class refresh(url, type, feed.get()));

            app_.submit(this, std::move(action));

            ++pending_;
        }
    }

    return (currently_pending != pending_);
}

bool rss::save_nzb(int row, const QString& folder)
{
    Q_ASSERT(row >= 0);
    Q_ASSERT(row < items_.size());

    const auto& item = items_[row];
    const auto& link = item.nzbLink;
    const auto& name = item.title;
    const auto& file = QDir::toNativeSeparators(folder + "/" + name + ".nzb");

    std::unique_ptr<class savenzb> action(new class savenzb(link, file));

    app_.submit(this, std::move(action));

    ++pending_;    

    return true;
}

void rss::set_params(const QString& site, const QVariantMap& values)
{
    auto it = std::find_if(std::begin(feeds_), std::end(feeds_),
        [=](const std::unique_ptr<feed>& feed) {
            return feed->name() == site;
        });
    Q_ASSERT(it != std::end(feeds_));

    auto& ptr = *it;

    ptr->set_params(values);
}

void rss::enable(const QString& site, bool val)
{
    enabled_[site] = val;
}

void rss::stop()
{
    app_.cancel_all(this);
    
    pending_ = 0;
    if (on_ready)
        on_ready();
}

void rss::download(int row, const QString& folder)
{
    Q_ASSERT(row >= 0);
    Q_ASSERT(row < items_.size());    
}

QAbstractItemModel* rss::view() 
{
    return this;
}

QVariant rss::data(const QModelIndex& index, int role) const
{
    const auto row = index.row();
    const auto col = columns(index.column());        
    const auto& item = items_[row];

    if (role == Qt::DisplayRole)
    {
        const auto& now = QDateTime::currentDateTime();

        switch (col)
        {
            case columns::date:
                return format(app::event {item.pubdate, now}); 

            case columns::category:
                return str(item.type);

            case columns::title:
                return item.title;

            case columns::size:
                if (item.size == 0)
                    return "n/a";
                return format(size { item.size });

            default:
                Q_ASSERT(!"unknown column");
                break;
        }
    }
    else if (role == Qt::DecorationRole)
    {
        if (col  == columns::date)
            return QIcon(":/resource/16x16_ico_png/ico_rss.png");
        else if (col == columns::title && item.password)
            return QIcon(":/resource/16x16_ico_png/ico_password.png");
    }
    return QVariant();
}

QVariant rss::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (columns(section))
    {
        case columns::date:     return "Date";
        case columns::category: return "Category";
        case columns::size:     return "Size";
        case columns::title:    return "Title";
        default:
        Q_ASSERT(!"unknown column");
        break;
    }
    return QVariant();
}

void rss::sort(int column, Qt::SortOrder order)
{
    emit layoutAboutToBeChanged();

    #define SORT(x) \
        std::sort(std::begin(items_), std::end(items_), \
            [&](const item& lhs, const item& rhs) { \
                if (order == Qt::AscendingOrder) \
                    return lhs.x < rhs.x; \
                return lhs.x > rhs.x; \
            });
        switch (columns(column))
        {
            case columns::date:     SORT(pubdate); break;
            case columns::category: SORT(type);    break;
            case columns::size:     SORT(size);    break;
            case columns::title:    SORT(title);   break;
            default:
            Q_ASSERT(!"unknown column");
            break;
        }
    #undef SORT

    emit layoutChanged();
}

int rss::rowCount(const QModelIndex&) const
{
    return (int)items_.size();
}

int rss::columnCount(const QModelIndex&) const
{
    return (int)columns::sentinel;
}

void rss::complete(std::unique_ptr<netreq> request) 
{
    if (class refresh* action = dynamic_cast<class refresh*>(request.get()))
    {
        const auto& items = action->items();
        const auto count  = items.size();

        beginInsertRows(QModelIndex(), items_.size(), items_.size() + count);
        std::copy(std::begin(items), std::end(items), std::back_inserter(items_));
        endInsertRows();
    }
    else if (class savenzb* action = dynamic_cast<class savenzb*>(request.get()))
    {
        DEBUG("Save nzb complete");
    }
    else if (class download* action = dynamic_cast<class download*>(request.get()))
    {
        DEBUG("Download nzb complete");
    }

    if (--pending_ == 0)
    {
        if (on_ready)
            on_ready();
    }
}

void rss::clear()
{
    items_.clear();
    QAbstractTableModel::reset();                
}

} // app
