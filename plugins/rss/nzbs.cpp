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

//#define PLUGIN_IMPL
//#define LOGTAG "nzbs"

#define LOGTAG "rss"

#include <newsflash/sdk/eventlog.h>
#include <newsflash/sdk/debug.h>
#include <newsflash/sdk/format.h>

#include <newsflash/warnpush.h>
#  include <QtXml/QDomDocument>
#  include <QDateTime>
#  include <QUrl>
#  include <QByteArray>
#  include <QBuffer>
#include <newsflash/warnpop.h>

#include "nzbs.h"
#include "rss.h"

using sdk::str;

namespace rss
{

nzbs::nzbs()
{
    DEBUG("nzbs created");
}

nzbs::~nzbs()
{
    DEBUG("nzbs destroyed");
}

bool nzbs::parse(QIODevice& io, std::vector<sdk::rssfeed::item>& rss) const
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

        sdk::rssfeed::item item {};
        item.title   = elem.firstChildElement("title").text();
        item.id      = elem.firstChildElement("guid").text();
        item.nzb     = elem.firstChildElement("link").text();
        item.pubdate = rss::parse_date(elem.firstChildElement("pubDate").text());

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
        if (item.nzb.contains("action=view&"))
            item.nzb.replace("action=view&", "action=getnzb&");

        rss.push_back(std::move(item));
    }

    return true;
}

void nzbs::prepare(sdk::category cat, std::vector<QUrl>& urls) const
{
    struct feed {
        sdk::category cat;
        const char* arg;
    } feeds[] = {
        {sdk::category::console_nds, "1010"},
        {sdk::category::console_psp, "1020"},
        {sdk::category::console_wii, "1030"},
        {sdk::category::console_xbox, "1040"},
        {sdk::category::console_xbox360, "1050"},
        {sdk::category::console_ps3, "1080"},
        {sdk::category::console_ps2, "1090"},
        {sdk::category::movies_int, "2060"}, // foreign
        {sdk::category::movies_sd, "2010"}, // dvd
        {sdk::category::movies_hd, "2040"}, // x264
        {sdk::category::movies_hd, "2020"}, // wmv-hd
        {sdk::category::audio_mp3, "3010"},
        {sdk::category::audio_video, "3020"}, 
        {sdk::category::audio_lossless, "3040"}, // flac
        {sdk::category::apps_pc, "4010"}, // 0day
        {sdk::category::apps_iso, "4020"},
        {sdk::category::apps_mac, "4030"},
        {sdk::category::apps_android, "4070"},
        {sdk::category::apps_ios, "4060"},
        {sdk::category::tv_int, "5080"}, // foreign
        {sdk::category::tv_sd, "5010"}, // dvd
        {sdk::category::tv_sd, "5070"}, // boxsd
        {sdk::category::tv_hd, "5040"}, // hd
        {sdk::category::tv_hd, "5090"}, // boxhd
        {sdk::category::tv_other, "5060"},
        {sdk::category::xxx_dvd, "6010"},
        {sdk::category::xxx_hd, "6040"}, // x264
        {sdk::category::xxx_sd, "6030"}, // xvid
        {sdk::category::other, "7010"} // other misc
    };

    Q_ASSERT(feedsize_);
    Q_ASSERT(!userid_.isEmpty());
    Q_ASSERT(!apikey_.isEmpty());

    const QString site("http://nzbs.org/rss?t=%1&num=%2&i=%3&r=%4&dl=1");

    for (const auto& feed : feeds)
    {
        if (feed.cat == cat)
        {
            QUrl url(site.arg(feed.arg)
                .arg(feedsize_)
                .arg(userid_)
                .arg(apikey_));
            urls.push_back(url);
        }
    }
}

bool nzbs::set_params(const QVariantMap& params)
{
    const auto userid   = params["userid"].toString();
    const auto apikey   = params["apikey"].toString();
    const auto feedsize = params["feedsize"].toInt();

    if (feedsize == 0)
        return false;
    if (userid.isEmpty())
        return false;
    if (apikey.isEmpty())
        return false;

    userid_   = userid;
    apikey_   = apikey;
    feedsize_ = feedsize;

    return true;
}

QString nzbs::site() const
{
    return "http://nzbs.org";
}

QString nzbs::name() const
{
    return "nzbs";
}



} // rss
