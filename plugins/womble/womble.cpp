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

#define MODEL_IMPL
#define LOGTAG "womble"

#include <newsflash/sdk/eventlog.h>
#include <newsflash/sdk/debug.h>
#include <newsflash/sdk/format.h>

#include <newsflash/warnpush.h>
#  include <QtNetwork/QNetworkRequest>
#  include <QtNetwork/QNetworkReply>
#  include <QtXml/QDomDocument>
#  include <QIODevice>
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

void plugin::rss::prepare(QNetworkRequest& request)
{
    request.setUrl(url_);
}

void plugin::rss::receive(QNetworkReply& reply)
{
    if (reply.error() != QNetworkReply::NoError)
        return;

    QByteArray bytes = reply.readAll();
    QBuffer io(&bytes);

    QDomDocument dom;
    QString error_string;
    int     error_line;
    int     error_column;
    if (!dom.setContent(&io, false, &error_string, &error_line, &error_column))
    {
        ERROR(str("Error parsing RSS response '_1'", error_string));
        return;
    }

    const auto& root  = dom.firstChildElement();
    const auto& items = root.elementsByTagName("item");
    for (int i=0; i<items.size(); ++i)
    {
        const auto& elem = items.at(i).toElement();

        plugin::item item {};
        item.title = elem.firstChildElement("title").text();
        item.id    = elem.firstChildElement("link").text();
        item.date  = ::rss::parse_date(elem.firstChildElement("pubDate").text());
        items_.push_back(std::move(item));
    }

    DEBUG(str("Parse _1 items from RSS stream", items_.size()));
}

} // womble


