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

#define LOGTAG "newznab"

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QtXml/QDomDocument>
#  include <QIODevice>
#  include <QUrl>
#  include <QBuffer>
#include "newsflash/warnpop.h"

#include <algorithm>
#include <string>

#include "eventlog.h"
#include "newznab.h"
#include "debug.h"
#include "rssdate.h"
#include "webquery.h"
#include "webengine.h"
#include "format.h"

namespace {

// mapping table
struct Mapping {
    app::MediaType type;
    const char* arg;
} mappings[] = {
    {app::MediaType::GamesNDS,      "1010"},
    {app::MediaType::GamesPSP,      "1020"},
    {app::MediaType::GamesWii,      "1030"},
    {app::MediaType::GamesXbox,     "1040"},
    {app::MediaType::GamesXbox360,  "1050"},
    {app::MediaType::GamesPS3,      "1080"},
    {app::MediaType::GamesPS2,      "1090"},

    {app::MediaType::MoviesInt,     "2060"}, // foreign
    {app::MediaType::MoviesSD,      "2010"}, // dvd
    {app::MediaType::MoviesWMV,     "2020"}, // wmv-hd
    {app::MediaType::MoviesSD,      "2030"}, // xvid
    {app::MediaType::MoviesHD,      "2040"}, // x264

    {app::MediaType::MusicMp3,      "3010"},
    {app::MediaType::MusicVideo,    "3020"},
    {app::MediaType::MusicLossless, "3040"}, // flac

    {app::MediaType::AppsPC,        "4010"}, // 0day
    {app::MediaType::AppsISO,       "4020"},
    {app::MediaType::AppsMac,       "4030"},
    {app::MediaType::AppsIos,       "4060"},
    {app::MediaType::AppsAndroid,   "4070"},

    {app::MediaType::TvSD,          "5010"}, // dvd
    {app::MediaType::TvHD,          "5040"}, // hd
    {app::MediaType::TvSD,          "5070"}, // boxsd
    {app::MediaType::TvInt,         "5080"}, // foreign
    {app::MediaType::TvHD,          "5090"}, // boxhd

    {app::MediaType::AdultOther,    "6000"},
    {app::MediaType::AdultDVD,      "6010"},
    {app::MediaType::AdultHD,       "6040"}, // x264
    {app::MediaType::AdultSD,       "6030"}, // xvid
};

app::MediaType mapType(unsigned code)
{
    auto it = std::find_if(std::begin(mappings), std::end(mappings),
        [=](const Mapping& m) {
            return std::stoul(m.arg) == code;
        });
    if (it == std::end(mappings))
    {
        switch (code / 1000)
        {
            case 1: return app::MediaType::GamesOther;
            case 2: return app::MediaType::MoviesOther;
            case 3: return app::MediaType::MusicOther;
            case 4: return app::MediaType::AppsOther;
            case 5: return app::MediaType::TvOther;
            case 6: return app::MediaType::AdultOther;
        }
        WARN("Newznab code %1 could not be mapped.", code);
        return app::MediaType::Other;
    }
    return (*it).type;
}

bool parseResponse(QNetworkReply& reply, app::newznab::HostInfo& info, QDomDocument& dom)
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

QString makeRssUrl(QString host)
{
    QUrl url(host);
    const auto& path = url.path();
    if (path.isEmpty())
        host += "/rss/";
    else if (path == "/")
        host += "rss/";
    else if (path == "/api/")
        host.replace("/api/", "/rss/");
    else if (path == "api/")
        host.replace("api/", "rss/");

    return host;
}

QString slashCombine(const QString& lhs, const QString& rhs)
{
    if (lhs.endsWith("/") || rhs.startsWith("/"))
        return lhs + rhs;

    return lhs + "/" + rhs;
}

} // namespace

namespace app
{

namespace newznab {

Indexer::Error Search::parse(QIODevice& io, std::vector<app::MediaItem>& results)
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

        const QString& link = elem.firstChildElement("link").text();
        if (link.startsWith("https://") || link.startsWith("http://"))
        {
            item.nzblink = link;
        }
        else
        {
            // nzbsooti.sx does not provide whole urls.
            QString host = apiurl_;
            if (host.endsWith("/"))
                host.chop(1);
            if (host.endsWith("api"))
                host.chop(3);

            item.nzblink = slashCombine(host, link);
        }


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

void Search::prepare(const BasicQuery& query, QUrl& url)
{
    url.setUrl(makeApiUrl(apiurl_));
    url.addQueryItem("apikey", apikey_);
    url.addQueryItem("extended", "1");
    url.addQueryItem("t", "search");
    url.addQueryItem("offset", QString::number(query.offset));
    if (!query.keywords.isEmpty())
        url.addQueryItem("q", query.keywords);
}

void Search::prepare(const AdvancedQuery& query, QUrl& url)
{
    QStringList cats;
    for (unsigned i=0; i<query.categories.BitCount; ++i)
    {
        if (!query.categories.test(i))
            continue;

        const auto type = static_cast<app::MainMediaType>(i);
        for (const auto& mapping : mappings)
        {
            if (!categories_.test(mapping.type))
                continue;

            if (toMainType(mapping.type) != type)
                continue;

            cats << mapping.arg;
        }
    }
    QString categories;
    categories = cats.join(",");

    url.setUrl(makeApiUrl(apiurl_));
    url.addQueryItem("apikey", apikey_);
    url.addQueryItem("extended", "1");
    url.addQueryItem("t", "search");
    url.addQueryItem("offset", QString::number(query.offset));
    url.addQueryItem("cat", categories);
    if (!query.keywords.isEmpty())
        url.addQueryItem("q", query.keywords);
}

void Search::prepare(const MusicQuery& query, QUrl& url)
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

void Search::prepare(const TelevisionQuery& query, QUrl& url)
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

QString Search::name() const
{
    return apiurl_;
}

bool RSSFeed::parse(QIODevice& io, std::vector<MediaItem>& rss)
{
    QDomDocument dom;
    QString error_string;
    int error_line   = 0;
    int error_column = 0;
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
        item.title   = elem.firstChildElement("title").text();
        item.guid    = elem.firstChildElement("guid").text();
        item.nzblink = elem.firstChildElement("link").text();
        item.pubdate = parseRssDate(elem.firstChildElement("pubDate").text());

        const auto& attrs = elem.elementsByTagName("newznab:attr");
        for (int x=0; x<attrs.size(); ++x)
        {
            const auto& attr = attrs.at(x).toElement();
            const auto& name = attr.attribute("name");
            if (name == "size")
                item.size = attr.attribute("value").toLongLong();
            else if (name == "password")
                item.password = attr.attribute("value").toInt();
            else if (name =="category")
                item.type = mapType(attr.attribute("value").toInt());
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

void RSSFeed::prepare(MainMediaType type, std::vector<QUrl>& urls)
{
    // nzbs.org supports num=123 parameter for limiting the feed size
    // however since we map multiple rss feeds to a single category
    // it doesn't work as the user expects.
    // num= defaults to 25. 100 is the max value. we'll use that.
    for (const auto& mapping : mappings)
    {
        if (!categories_.test(mapping.type))
            continue;

        if (toMainType(mapping.type) != type)
            continue;

        QUrl url;
        url.setUrl(makeRssUrl(apiurl_));
        url.addQueryItem("t", mapping.arg);
        url.addQueryItem("i", userid_);
        url.addQueryItem("r", apikey_);
        url.addQueryItem("dl", "1");
        url.addQueryItem("num", "100");
        urls.push_back(url);
    }
}

QString RSSFeed::name() const
{
    return apiurl_;
}


// global function
WebQuery* testAccount(const Account& acc, const HostCallback& callback)
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
        callback(info);
    };
    return g_web->submit(std::move(query));
}

WebQuery* registerAccount(const Account& acc, const HostCallback& callback)
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
            const auto& reg  = dom.firstChildElement("register");
            info.username    = reg.attribute("username");
            info.password    = reg.attribute("password");
            info.apikey      = reg.attribute("apikey");
            info.userid      = reg.attribute("userid");
            info.success = true;
        }
        callback(info);
    };
    return g_web->submit(std::move(query));
}

WebQuery* importServerList(const ImportCallback& callback)
{
    WebQuery query(QUrl("http://www.ensisoft.com/import.php"));
    query.OnReply = [=](QNetworkReply& reply) {
        ImportList list;

        const auto err = reply.error();
        if (err != QNetworkReply::NoError)
        {
            list.success = false;
            list.error   = toString(err);
            callback(list);
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
            if (host.startsWith("http://") || host.startsWith("https://"))
                list.hosts.push_back(host);
        }
        list.success = true;
        callback(list);
    };
    return g_web->submit(std::move(query));
}

} // newznab

} // app