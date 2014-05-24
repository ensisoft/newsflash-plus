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

#include <newsflash/gui/sdk/newsflash.h>
#include <newsflash/gui/sdk/rssrequest.h>
#include <newsflash/gui/sdk/nzbrequest.h>
#include <newsflash/gui/sdk/release.h>
#include <rsslib/rss.h>
#include <newsflash/warnpush.h>
#  include <QtNetwork/QNetworkRequest>
#  include <QtXml/QDomDocument>
#  include <QIODevice>
#  include <QDateTime>
#  include <QUrl>
#include <newsflash/warnpop.h>
#include <cstring>
#include "womble.h"

namespace {

    class womble_rss_request : public sdk::rssrequest 
    {
    public:
        womble_rss_request(sdk::mediatype type, const QString& url) 
          :  type_(type), url_(url)
        {}

       ~womble_rss_request()
        {}

        void prepare(QNetworkRequest& req)
        {
            req.setUrl(QUrl(url_));
        }
        bool receive(QIODevice& reply)
        {
            error_.clear();
            releases_.clear();

            QDomDocument dom;
            QString error_string;
            int     error_line   = 0;
            int     error_column = 0;
            if (!dom.setContent(&reply, false, &error_string, &error_line, &error_column))
            {
                error_ = QString("malformed RSS feed: \"%1\"").arg(error_string);
                return false;
            }
            const QDomElement& root   = dom.firstChildElement();
            const QDomNodeList& items = root.elementsByTagName("item");
            for (int i=0; i<items.size(); ++i)
            {
                const QDomElement& elem = items.at(i).toElement();
                sdk::release rel;
                rel.type       = type_;
                rel.size       = elem.firstChildElement("report:size").text().toLongLong();
                rel.title      = elem.firstChildElement("title").text();
                rel.infolink   = ""; // not available
                rel.id         = elem.firstChildElement("link").text();
                rel.pubdate    = QDateTime::currentDateTime();
                rel.passworded = false;
                rel.nzburl     = rel.id;
                rel.size       = 0;
                
                const QString date = elem.firstChildElement("pubDate").text();
                if (!date.isEmpty())
                {
                    const QDateTime& dt = rss::parse_date(date);
                    if (dt.isValid())
                        rel.pubdate = dt.toLocalTime();
                }
                releases_.push_back(rel);
            }
            return true;
        }
        QString url() const
        {
            return url_;
        }
        QString site() const
        {
            return "http://newshost.co.za";
        }
        QString error() const
        {
            return error_;
        }
        
        void get_releases(QList<sdk::release>& list)
        {
            list = releases_;
        }
    private:
        const sdk::mediatype type_;
        const QString url_;

    private:
        QList<sdk::release> releases_;
        QString error_;        
    };
    
    class womble_nzb_request : public sdk::nzbrequest
    {
    public:
        womble_nzb_request(const QString& id) : id_(id)
        {}

       ~womble_nzb_request()
        {}

        void prepare(QNetworkRequest& req)
        {
            req.setUrl(id_);
        }
        bool receive(QIODevice& reply)
        {
            return false;
        }
        QString url() const
        {
            return id_;
        }
        QString site() const
        {
            return "http://newshost.co.za";
        }
        QString error() const
        {
            return "";
        }
    private:
        QString id_;
    };
}

namespace womble
{

plugin::plugin(sdk::newsflash* host) : host_(host)
{}

plugin::~plugin()
{}

QList<sdk::request*> plugin::get_rss(sdk::mediatype media)
{
    QList<sdk::request*> ret;
    
    using namespace sdk;

    struct mapping {
        sdk::mediatype type;
        const char* name;
    };    

    // map the media types to RSS streams

    static const mapping mappings[] = 
    {
        {mediatype::console_nds, "NDS"},
        {mediatype::console_psp, "PSP"},
        {mediatype::console_wii, "WII"},
        {mediatype::console_xbox, "XBOX"},
        {mediatype::console_xbox360, "XBOX360"},
        {mediatype::console_ps3, "PS3"},
        {mediatype::console_ps2, "PS2"},
        {mediatype::console_ps4, "PS4"},

        {mediatype::movies_sd, "DIVX"},
        {mediatype::movies_sd, "XVID"},
        {mediatype::movies_sd, "DVD-PAL"},
        {mediatype::movies_hd, "BLURAY"}, // better than X264

        {mediatype::audio_mp3, "MP3"},
        {mediatype::audio_video, "MVIDS"}, // MVID and MV are same as MVIDS

        {mediatype::apps_pc, "0-DAY"},
        {mediatype::apps_pc, "0DAY"},
        {mediatype::apps_pc, "0DAY-"},
        {mediatype::apps_pc, "ISO"},

        {mediatype::tv_sd, "TV-XVID"},
        {mediatype::tv_sd, "TV-DVDRIP"},
        {mediatype::tv_hd, "TV-x264"},

        {mediatype::xxx_dvd, "XXX"},

        {mediatype::other_ebook, "EBOOK"}
    };
    const QString womble("http://newshost.co.za/rss/?sec=%1&fr=false");

    for (auto it = std::begin(mappings); it != std::end(mappings); ++it)
    {
        const auto& map = *it;
        if (map.type == media && std::strlen(map.name))
        {
            request* req = new womble_rss_request(media, womble.arg(map.name));
            ret.append(req);
        }
    }
    return ret;
}

sdk::request* plugin::get_nzb(const QString& id, const QString& url)
{
    return new womble_nzb_request(url);
}


QString plugin::name() const 
{
    return "Womble";
}

QString plugin::host() const
{
    return "http://newshost.co.za";
}

sdk::bitflag_t plugin::features() const
{
    return sdk::plugin::has_rss;
}

} // womble


PLUGIN_API sdk::plugin* create_plugin(sdk::newsflash* host)
{
    return new womble::plugin(host);
}

PLUGIN_API int plugin_api_version()
{
    return sdk::PLUGIN_API_VERSION;
}

PLUGIN_API void plugin_lib_version(int* major, int* minor)
{
    *major = 0;
    *minor = 2;
}
