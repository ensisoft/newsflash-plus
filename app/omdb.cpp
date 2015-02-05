// Copyright (c) 2014 Sami Väisänen, Ensisoft 
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

#define LOGTAG "omdb"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QJson/Parser>
#  include <QString>
#  include <QUrl>
#  include <QByteArray>
#  include <QBuffer>
#include <newsflash/warnpop.h>

#include "omdb.h"
#include "debug.h"
#include "eventlog.h"
#include "format.h"

namespace app
{

MovieDatabase::MovieDatabase()
{
    DEBUG("MovieDatabase created");

    net_ = g_net->getSubmissionContext();
}

MovieDatabase::~MovieDatabase()
{
    DEBUG("MovieDatabase deleted");
}

void MovieDatabase::beginLookup(const QString& title)
{
    QUrl url;
    url.setUrl("http://www.omdbapi.com");
    url.addQueryItem("t", title);
    url.addQueryItem("plot", "short");
    url.addQueryItem("r", "json");
    g_net->submit(std::bind(&MovieDatabase::onLookupFinished, this,
        std::placeholders::_1, title), net_, url);

    DEBUG(str("Lookup movie '_1'", title));
}

void MovieDatabase::abortLookup(const QString& title)
{}

const MovieDatabase::Movie* MovieDatabase::getMovie(const QString& title) const
{
    const auto it = db_.find(title);
    if (it == std::end(db_))
        return nullptr;

    return &it->second;
}

void MovieDatabase::onLookupFinished(QNetworkReply& reply, const QString& title)
{
    const auto err = reply.error();
    //if (err != QNetworkReply::NoError)
    //{
    //    ERROR(str("Failed to retrieve movie details '_1'", title));
    //    return;
    //}
    QByteArray bytes = reply.readAll();

    qDebug() << bytes;

    QBuffer io(&bytes);

    bool success = true;

    QJson::Parser parser;
    QVariantMap json = parser.parse(&io, &success).toMap();
    if (!success)
    {
        ERROR(str("Incorrect JSON response from omdbapi.com"));
        return;
    }
    Movie movie;
    movie.title = title;
    movie.genre = json["Genre"].toString();
    movie.plot  = json["Plot"].toString();
    db_.insert(std::make_pair(title, movie));

    const auto& poster = json["Poster"].toString();
    DEBUG(str("Retrieving poster from _1", poster));

    QUrl url;
    url.setUrl(poster);
    g_net->submit(std::bind(&MovieDatabase::onPosterFinished, this,
        std::placeholders::_1, title), net_, url);

    emit lookupReady(title);

    DEBUG(str("Movie lookup finished. Stored details for '_1'", title));
}

void MovieDatabase::onPosterFinished(QNetworkReply& reply, const QString& title)
{
    const auto err = reply.error();
    if (err != QNetworkReply::NoError)
    {
        return;
    }

    auto it = db_.find(title);
    Q_ASSERT(it != std::end(db_));

    QByteArray bytes = reply.readAll();
    QPixmap& pixmap  = it->second.poster;

    if (!pixmap.loadFromData(bytes))
    {
        return;
    }

    emit posterReady(title);
}

MovieDatabase* g_movies;

} // app
