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
//#define LOGTAG "womble"
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
#  include <QRegExp>
#include <newsflash/warnpop.h>

#include <cstring>
#include "womble.h"
//#include "rss.h"

using sdk::str;

namespace {


QDateTime parse_date(const QString& str)
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

} // namespace

namespace rss
{

womble::womble()
{
    DEBUG("womble created");
}

womble::~womble()
{
    DEBUG("womble destroyed");
}

bool womble::parse(QIODevice& io, std::vector<sdk::rssfeed::item>& rss) const
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

        sdk::rssfeed::item item {};
        item.title    = elem.firstChildElement("title").text();
        item.id       = elem.firstChildElement("link").text();
        item.pubdate  = parse_date(elem.firstChildElement("pubDate").text());
        item.password = false;
        item.nzb      = item.id;
        item.size     = 0; // not known
        rss.push_back(std::move(item));
    }
    return true;
}

void womble::prepare(sdk::category cat, std::vector<QUrl>& urls) const 
{
    struct feed {
        sdk::category  tag;
        const char* arg;
    } feeds[] = {
        {sdk::category::console_nds, "nds"},
        {sdk::category::console_psp, "psp"},
        {sdk::category::console_wii, "wii"},
        {sdk::category::console_xbox, "xbox"},
        {sdk::category::console_xbox360, "xbox360"},
        {sdk::category::console_ps3, "ps3"},
        {sdk::category::console_ps2, "ps2"},
        {sdk::category::movies_sd, "divx"},
        {sdk::category::movies_sd, "xvid"},
        {sdk::category::movies_sd, "dvd-pal"},
        {sdk::category::movies_hd, "bluray"}, // better than x264
        {sdk::category::audio_mp3, "mp3"},
        {sdk::category::audio_video, "mvids"}, // mvid and mv are same as mvids
        {sdk::category::apps_pc, "0-day"},
        {sdk::category::apps_pc, "0day"},
        {sdk::category::apps_pc, "0day-"},
        {sdk::category::apps_iso, "iso"},
        {sdk::category::tv_sd, "tv-xvid"},
        {sdk::category::tv_sd, "tv-dvdrip"},
        {sdk::category::tv_hd, "tv-x264"},
        {sdk::category::xxx_dvd, "xxx"},
        {sdk::category::ebook, "ebook"}        
    };

    const QString site("http://newshost.co.za/rss/?sec=%1&fr=false");

    for (const auto& feed : feeds)
    {
        if (feed.tag == cat)
        {
            QUrl url(site.arg(feed.arg));
            urls.push_back(url);
        }
    }
}

QString womble::site() const
{
    return "http://newshost.co.za";
}

QString womble::name() const 
{
    return "womble";
}

} // rss

// PLUGIN_API sdk::rssfeed* create_rssfeed(int version)
// {
//     if (version != sdk::rssfeed::version)
//         return nullptr;

//     try
//     {
//         return new womble::womble();
//     }
//     catch (const std::exception& e)
//     {
// 	ERROR(str("Error creating womble womble _1", e.what()));
//     }
//     return nullptr;
// }
