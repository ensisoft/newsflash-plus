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

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtXml/QDomDocument>
#  include <QIODevice>
#  include <QDateTime>
#  include <QUrl>
#  include <QByteArray>
#  include <QBuffer>
#include <newsflash/warnpop.h>
#include "debug.h"
#include "format.h"
#include "nzbs.h"
#include "rssdate.h"

namespace app
{

bool nzbs::parse(QIODevice& io, std::vector<mediaitem>& rss)
{
    QDomDocument dom;
    QString error_string;
    int error_line   = 0;
    int error_column = 0;
    if (!dom.setContent(&io, false, &error_string, &error_line, &error_column))
    {
        DEBUG(str("XML parse error _1", error_string));
        DEBUG(str("Line: _1", error_line));
        return false;
    }
     
    const auto& root  = dom.firstChildElement();
    const auto& items = root.elementsByTagName("item");
    for (int i=0; i<items.size(); ++i)
    {
        const auto& elem = items.at(i).toElement();

        mediaitem item {};
        item.title   = elem.firstChildElement("title").text();
        item.gid     = elem.firstChildElement("guid").text();
        item.nzblink = elem.firstChildElement("link").text();
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
        if (item.nzblink.contains("action=view&"))
            item.nzblink.replace("action=view&", "action=getnzb&");

        rss.push_back(std::move(item));
    }
    return true;
}

void nzbs::prepare(media type, std::vector<QUrl>& urls)
{
    // mapping table
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
        {media::xxx_sd, "6030"} // xvid
      //{media::other, "7010"} // other misc
    };

    // see the RSS feed documentation over here
    // http://nzbs.org/rss
    // NOTE: the i= param is the USER ID which is not the same as the user name!!

    // nzbs.org supports num=123 parameter for limiting the feed size
    // however since we map multiple rss feeds to a single category
    // it doesn't work as the user expects.
    // num= defaults to 25. 100 is the max value. we'll use that.
    const QString site("http://nzbs.org/rss?t=%1&i=%2&r=%3&dl=1&num=100");

    for (const auto& feed : feeds)
    {
        if (feed.type == type)
        {

            // see comments about the feedsize num limit above.
            QUrl url(site.arg(feed.arg).arg(userid_).arg(apikey_));
            urls.push_back(url);
        }
    }        
}

} // app