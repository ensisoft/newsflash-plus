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

#include <stdexcept>

#include "womble.h"
#include "debug.h"
#include "format.h"

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

namespace app
{

bool womble::parse(QIODevice& io, std::vector<mediaitem>& rss)
{
    QDomDocument dom;
    QString error_string;
    int error_line;
    int error_column;
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
        item.title    = elem.firstChildElement("title").text();
        item.gid      = elem.firstChildElement("link").text();
        item.pubdate  = parse_date(elem.firstChildElement("pubDate").text());
        item.password = false;
        item.nzblink  = item.gid;
        item.size     = 0; // not known
        rss.push_back(std::move(item));
    }
    return true;
}

void womble::prepare(media m, std::vector<QUrl>& urls)
{
    // our mapping table
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
        if (feed.type == m)
        {
            QUrl url(site.arg(feed.arg));
            urls.push_back(url);
        }
    }            
}

} // app