// Copyright (c) 2010-2015 Sami Väisänen, Ensisoft 
//
// http://www.ensisoft.com
// 
// This software is copyrighted software. Unauthorized hacking, cracking, distribution
// and general assing around is prohibited.
// Redistribution and use in source and binary forms, with or without modification,
// without permission are prohibited.
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

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

QDateTime parseDate(const QString& str)
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

bool Womble::parse(QIODevice& io, std::vector<MediaItem>& rss)
{
    QDomDocument dom;
    QString error_string;
    int error_line;
    int error_column;
    if (!dom.setContent(&io, false, &error_string, &error_line, &error_column))
    {
        DEBUG("XML parse error '%1' on line %2", error_string, error_line);
        return false;
    }

    const auto& root  = dom.firstChildElement();
    const auto& items = root.elementsByTagName("item");
    for (int i=0; i<items.size(); ++i)
    {
        const auto& elem = items.at(i).toElement();

        MediaItem item {};
        item.title    = elem.firstChildElement("title").text();
        item.guid     = elem.firstChildElement("link").text();
        item.pubdate  = parseDate(elem.firstChildElement("pubDate").text());
        item.password = false;
        item.nzblink  = item.guid;
        item.size     = 0; // not known
        rss.push_back(std::move(item));
    }
    return true;
}

void Womble::prepare(MediaType m, std::vector<QUrl>& urls)
{
    using Media = MediaType;
    // our mapping table
    struct feed {
        Media type;
        const char* arg;
    } feeds[] = {
        {Media::GamesNDS, "nds"},
        {Media::GamesPSP, "psp"},
        {Media::GamesWii, "wii"},
        {Media::GamesXbox, "xbox"},
        {Media::GamesXbox360, "xbox360"},
        {Media::GamesPS3, "ps3"},
        {Media::GamesPS2, "ps2"},
        {Media::MoviesSD, "divx"},
        {Media::MoviesSD, "xvid"},
        {Media::MoviesSD, "dvd-pal"},
        {Media::MoviesHD, "bluray"}, // better than x264
        {Media::MusicMp3, "mp3"},
        {Media::MusicVideo, "mvids"}, // mvid and mv are same as mvids
        {Media::AppsPC, "0-day"},
        {Media::AppsPC, "0day"},
        {Media::AppsPC, "0day-"},
        {Media::AppsISO, "iso"},
        {Media::TvSD, "tv-xvid"},
        {Media::TvSD, "tv-dvdrip"},
        {Media::TvHD, "tv-x264"},
        {Media::AdultDVD, "xxx"},
        {Media::Ebook, "ebook"}        
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