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
#include <newsflash/warnpop.h>
#include <algorithm>
#include "newznab.h"
#include "debug.h"
#include "rssdate.h"

namespace {

using Type = app::MediaType;
using Code = unsigned;

struct Mapping {
    Type type;
    Code code;
} map[] = {
    {Type::ConsoleNDS, 1010},
    {Type::ConsoleWii, 1030},
    {Type::ConsoleXbox, 1040},
    {Type::ConsoleXbox360, 1050},
    {Type::ConsolePSP, 1020},

    {Type::MoviesInt, 2010},
    {Type::MoviesSD, 2030},
    {Type::MoviesHD, 2040}
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

bool Newznab::prepare(const BasicQuery& query, QUrl& url)
{
    url.setHost(apiurl_);
    url.addQueryItem("apikey", apikey_);
    url.addQueryItem("extended", "1");
    url.addQueryItem("t", "search");
    url.addQueryItem("q", query.keywords);
    return true;
}

void Newznab::setParams(const Params& params)
{
    apikey_ = params.apikey;
    apiurl_ = params.apiurl;
}

} // app