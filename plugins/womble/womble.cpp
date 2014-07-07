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

#define LOGTAG "womble"

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

#include <newsflash/rsslib/rss.h>

#include <cstring>
#include "womble.h"


using sdk::str;

namespace womble
{

plugin::plugin()
{
    DEBUG(str("womble _1 created", (const void*)this));
}

plugin::~plugin()
{
    DEBUG(str("womble _1 destroyed", (const void*)this));
}

bool plugin::parse(QIODevice& io, std::vector<sdk::rssfeed::item>& rss) const
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
        item.pubdate  = rss::parse_date(elem.firstChildElement("pubDate").text());
        item.password = false;
        item.nzb      = item.id;
        item.size     = 0; // not known
        rss.push_back(item);
    }
    return true;
}

void plugin::prepare(sdk::category cat, std::vector<QUrl>& urls) const 
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

QString plugin::site() const
{
    return "http://newshost.co.za";
}

QString plugin::name() const 
{
    return "womble";
}

} // womble

PLUGIN_API sdk::rssfeed* create_rssfeed(int version)
{
    if (version != sdk::rssfeed::version)
        return nullptr;

    try
    {
        return new womble::plugin();
    }
    catch (const std::exception& e)
    {
	ERROR(str("Error creating womble plugin _1", e.what()));
    }
    return nullptr;
}
