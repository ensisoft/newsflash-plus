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
#  include <QUrl>
#  include <QBuffer>
#include <newsflash/warnpop.h>
#include <algorithm>
#include "newznab.h"
#include "debug.h"
#include "rssdate.h"
#include "webquery.h"
#include "webengine.h"
#include "format.h"

namespace {

using Type = app::MediaType;
using Cat  = app::Indexer::Category;
using Code = unsigned;

struct Mapping {
    Type type;
    Code code;
    Cat  cat;
} map[] = {
    {Type::ConsoleNDS, 1010, Cat::Console},
    {Type::ConsoleWii, 1030, Cat::Console},
    {Type::ConsoleXbox, 1040, Cat::Console},
    {Type::ConsoleXbox360, 1050, Cat::Console},
    {Type::ConsolePSP, 1020, Cat::Console},
    {Type::ConsolePS2, 1080, Cat::Console},    
    {Type::ConsolePS3, 1090, Cat::Console},    
    //{Type::ConsolePS4, ??},        

    {Type::MoviesInt, 2060, Cat::Movies},
    {Type::MoviesSD, 2010, Cat::Movies},
    {Type::MoviesHD, 2040, Cat::Movies},

    {Type::AudioMp3, 3010, Cat::Music},
    {Type::AudioVideo, 3020, Cat::Music},
    {Type::AudioAudiobook, 3030, Cat::Music},
    {Type::AudioLossless, 3040, Cat::Music},

    {Type::AppsPC, 4000, Cat::Apps},
    {Type::AppsISO, 4020, Cat::Apps},
    {Type::AppsMac, 4030, Cat::Apps},
    {Type::AppsAndroid, 4070, Cat::Apps},
    {Type::AppsIos, 4060, Cat::Apps},

    {Type::TvInt, 5020, Cat::Television},
    {Type::TvSD, 5030, Cat::Television},
    {Type::TvHD, 5040, Cat::Television},
    {Type::TvOther, 5050, Cat::Television},
    {Type::TvSport, 5060, Cat::Television},

    {Type::XxxDVD, 6010, Cat::Porno},
    {Type::XxxHD, 6040, Cat::Porno},
    {Type::XxxSD, 6030, Cat::Porno},

    {Type::Ebook, 7020, Cat::Other}
};

app::MediaType mapType(unsigned code)
{
    auto it = std::find_if(std::begin(map), std::end(map),
        [=](const Mapping& m) {
            return m.code == code;
        });
    if (it == std::end(map))
        return app::MediaType::Other;
    return (*it).type;
}

void collect(QStringList& str, Cat c)
{
    for (auto beg = std::begin(map); beg != std::end(map); ++beg)
    {
        const auto& item =*beg;
        if (item.cat != c)
            continue;
        str << QString::number(item.code);
    }
}

QString collect(newsflash::bitflag<app::Indexer::Category> bits)
{
    QStringList str;
    for (unsigned i=0; i<bits.BitCount; ++i)
    {
        if (!bits.test(i))
            continue;

        const auto cat = static_cast<Cat>(i);
        collect(str, cat);
    }
    return str.join(",");
}

bool parseResponse(QNetworkReply& reply, app::Newznab::HostInfo& info, QDomDocument& dom)
{
    const auto err = reply.error();
    if (err != QNetworkReply::NoError)
    {
        info.error   = app::toString(err);
        info.success = false;
        return false;
    }

    auto buff = reply.readAll();
    QBuffer io(&buff);

    QString errorString;
    int errorRow;
    int errorCol;
    if (!dom.setContent(&io, false, &errorString, &errorRow, &errorCol))
    {
        info.error = "There was an error reading the response data.\n"
            "Perhaps this host is not a Newznab provider?";
        info.success = false;
        return false;
    }
    QDomElement error = dom.firstChildElement("error");
    if (!error.isNull())
    {
        const auto code = error.attribute("code");
        const auto desc = error.attribute("description");
        switch (code.toInt())
        {
            case 100: info.error = "Incorrect Credentials"; break;
            case 101: info.error = "Account suspended"; break;
            case 102: info.error = "No permission"; break;
            default:  info.error = desc;
        }
        info.success = false;
        return false;
    }
    return true;
}

QString makeApiUrl(QString host)
{
    QUrl url(host);
    const auto path = url.path();
    if (path.isEmpty())
        host += "/api/";
    else if (path == "/")
        host += "api/";

    return host;
}

} // namespace 

namespace app
{

Indexer::Error Newznab::parse(QIODevice& io, std::vector<MediaItem>& results)
{
    int errorColumn;
    int errorLine;
    QString errorString;

    QDomDocument dom;
    if (!dom.setContent(&io, false, &errorString, &errorLine, &errorColumn))
    {
        DEBUG("XML parse error '%1' on line %2", errorString, errorLine);
        return Error::Content;
    }

    QDomElement error = dom.firstChildElement("error");
    if (!error.isNull())
    {
        const auto code = error.attribute("code");
        const auto desc = error.attribute("description");
        DEBUG("Error response %1 %2", code, desc);
        switch (code.toInt())
        {
            case 100: return Error::IncorrectCredentials;
            case 101: return Error::AccountSuspended;
            case 102: return Error::NoPermission;
        }
        return Error::Unknown;
    }

    const QDomElement root   = dom.firstChildElement();
    const QDomNodeList resp  = root.elementsByTagName("newznab:response");
    const QDomNodeList items = root.elementsByTagName("item");    

    for (int i=0; i<items.size(); ++i)
    {
        const QDomElement elem = items.at(i).toElement();
        if (elem.isNull())
            continue;

        MediaItem item;
        item.size     = 0;
        item.password = false;
        item.title    = elem.firstChildElement("title").text();

        // there's a newznab bug (11/7/2010) so that if the search matches no data
        // a single incomplete <item> is returned. therefore we check if the title for 
        // emptyness to work around this issue.
        if (item.title.isEmpty())
            continue;

        item.nzblink = elem.firstChildElement("link").text();

        // the GUID is RSS is in HTTP URL format... we only need the ID part.
        // for example: http://nzb.su/details/51216cf764458c19d61fc0ede42ad52c
        // id: 51216cf764458c19d61fc0ede42ad52c
        auto guid  = elem.firstChildElement("link").text();
        auto slash = guid.lastIndexOf("/");
        if (slash != -1)
            guid = guid.right(guid.size() - slash - 1);

        item.guid = guid;

        // check some interesting extended attributes.
        const QDomNodeList attrs = elem.elementsByTagName("newznab:attr");
        for (int i=0; i<attrs.size(); ++i)
        {
            const QDomElement elem = attrs.at(i).toElement();
            if (elem.isNull())
                continue;

            const auto name = elem.attribute("name");
            if (name == "size")
                item.size = elem.attribute("value").toULongLong();
            else if (name == "password")
                item.password = elem.attribute("value").toInt();
            else if (name == "usenetdate")
                item.pubdate = parseRssDate(elem.attribute("value"));
            else if (name == "category")
                item.type = mapType(elem.attribute("value").toInt());
        }
        results.push_back(item);
    }

    return Error::None;
}

void Newznab::prepare(const BasicQuery& query, QUrl& url)
{
    url.setUrl(makeApiUrl(apiurl_));
    url.addQueryItem("apikey", apikey_);
    url.addQueryItem("extended", "1");
    url.addQueryItem("t", "search");
    url.addQueryItem("offset", QString::number(query.offset));
    if (!query.keywords.isEmpty())
        url.addQueryItem("q", query.keywords);
}

void Newznab::prepare(const AdvancedQuery& query, QUrl& url)
{
    url.setUrl(makeApiUrl(apiurl_));
    url.addQueryItem("apikey", apikey_);
    url.addQueryItem("extended", "1");
    url.addQueryItem("t", "search");
    url.addQueryItem("offset", QString::number(query.offset));    
    url.addQueryItem("cat", collect(query.categories));
    if (!query.keywords.isEmpty())
        url.addQueryItem("q", query.keywords);
}

void Newznab::prepare(const MusicQuery& query, QUrl& url)
{
    url.setUrl(makeApiUrl(apiurl_));
    url.addQueryItem("apikey", apikey_);
    url.addQueryItem("extended", "1");
    url.addQueryItem("t", "music");
    url.addQueryItem("offset", QString::number(query.offset));    
    if (!query.artist.isEmpty())
        url.addQueryItem("artist", query.artist);
    if (!query.album.isEmpty())
        url.addQueryItem("album", query.album);
    if (!query.track.isEmpty())
        url.addQueryItem("track", query.track);
    if (!query.year.isEmpty())
        url.addQueryItem("year", query.year);
}

void Newznab::prepare(const TelevisionQuery& query, QUrl& url)
{
    url.setUrl(makeApiUrl(apiurl_));
    url.addQueryItem("apikey", apikey_);
    url.addQueryItem("extended", "1");
    url.addQueryItem("t", "tvsearch");
    url.addQueryItem("offset", QString::number(query.offset));    
    if (!query.keywords.isEmpty())
        url.addQueryItem("q", query.keywords);
    if (!query.episode.isEmpty())
        url.addQueryItem("ep", query.episode);
    if (!query.season.isEmpty())
        url.addQueryItem("season", query.season);
}

QString Newznab::name() const
{
    return apiurl_;
}

void Newznab::setAccount(const Account& acc)
{
    apikey_ = acc.apikey;
    apiurl_ = acc.apiurl;
}

WebQuery* Newznab::apiTest(const Account& acc, HostCallback cb)
{
    QUrl url(makeApiUrl(acc.apiurl));
    url.addQueryItem("apikey", acc.apikey);
    url.addQueryItem("t", "caps");

    WebQuery query(url);
    query.OnReply = [=](QNetworkReply& reply) {
        HostInfo info;            
        QDomDocument dom;
        if (parseResponse(reply, info, dom))
        {
            const auto& root   = dom.firstChildElement();
            const auto& server = root.firstChildElement("server");
            info.strapline = server.attribute("strapline");
            info.email     = server.attribute("email");
            info.version   = server.attribute("version");
            info.success   = true;
        }
        cb(info);
    };
    return g_web->submit(std::move(query));
}

WebQuery* Newznab::apiRegisterUser(const Account& acc, HostCallback cb)
{
    QUrl url(makeApiUrl(acc.apiurl));
    url.addQueryItem("apikey", acc.apikey);
    url.addQueryItem("t", "register");
    url.addQueryItem("email", acc.email);

    WebQuery query(url);
    query.OnReply = [=](QNetworkReply& reply) {
        HostInfo info;
        QDomDocument dom;
        if (parseResponse(reply, info, dom))
        {
            const auto& root = dom.firstChildElement();
            const auto& reg  = root.firstChildElement("register");
            info.username    = reg.attribute("username");
            info.password    = reg.attribute("password");
            info.apikey      = reg.attribute("apikey");
            info.success = true;
        }
        cb(info);
    };
    return g_web->submit(std::move(query));
}

WebQuery* Newznab::importList(ImportCallback cb)
{
    WebQuery query(QUrl("http://www.ensisoft.com/import.php"));
    query.OnReply = [=](QNetworkReply& reply) {
        ImportList list;

        const auto err = reply.error();
        if (err != QNetworkReply::NoError)
        {
            list.success = false;
            list.error   = toString(err);
            cb(list);
            return;
        }
        const auto& buff   = reply.readAll();
        const auto& string = QString::fromUtf8(buff.constData(),
            buff.size());
        const auto& lines = string.split("\n", QString::SkipEmptyParts);

        for (const auto& line : lines)
        {
            const auto& fields = line.split("\t", QString::SkipEmptyParts);
            if (fields.isEmpty())
                continue;
            const auto& host = fields[0];
            if (host.startsWith("http://"))
                list.hosts.push_back(host);
        }
        list.success = true;
        cb(list);
    };
    return g_web->submit(std::move(query));
}

} // app