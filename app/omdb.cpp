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
    QUrl url("http://www.omdbapi.com/");
    url.addQueryItem("t", title);
    url.addQueryItem("y", "");
    url.addQueryItem("plot", "short");
    url.addQueryItem("r", "json");
    g_net->submit(std::bind(&MovieDatabase::onLookupFinished, this,
        std::placeholders::_1, title), net_, url);

    DEBUG("Lookup movie '%1'", title);
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
    if (err != QNetworkReply::NoError)
    {
       ERROR("Failed to retrieve movie details '%1', %2", title, err);
       emit lookupError(title, toString("Failed to retrieve movie details (%1)", err));
       return;
    }
    QByteArray bytes = reply.readAll();
    QBuffer io(&bytes);

    bool success = true;

    QJson::Parser parser;
    QVariantMap json = parser.parse(&io, &success).toMap();
    if (!success)
    {
        ERROR("Incorrect JSON response from omdbapi.com");
        emit lookupError(title, "Incorrect JSON response from omdbapi.com");
        return;
    }
    success = json["Response"].toBool();
    if (!success)
    {
        const auto& err = json["Error"].toString();
        ERROR("Error response from omdbapi.com. %1", err);
        emit lookupError(title, err);
        return;
    }

    Movie movie;
    movie.title = title;
    movie.genre = json["Genre"].toString();
    movie.plot  = json["Plot"].toString();
    movie.director = json["Director"].toString();
    movie.language = json["Language"].toString();
    movie.country  = json["Country"].toString();
    movie.rating   = json["imdbRating"].toFloat();
    movie.year     = json["Year"].toInt();
    movie.runtime  = json["Runtime"].toString();
    movie.actors   = json["Actors"].toString();
    movie.imdbid   = json["imdbID"].toString();
    db_.insert(std::make_pair(title, movie));

    const auto& poster = json["Poster"].toString();
    DEBUG("Retrieving poster from %1", poster);

    g_net->submit(std::bind(&MovieDatabase::onPosterFinished, this,
        std::placeholders::_1, title), net_, poster);

    emit lookupReady(title);

    DEBUG("Movie lookup finished. Stored details for '%1'", title);
}

void MovieDatabase::onPosterFinished(QNetworkReply& reply, const QString& title)
{
    const auto err = reply.error();
    if (err != QNetworkReply::NoError)
    {
        emit posterError(title, "Failed to retrieve the poster (network error)");
        return;
    }

    auto it = db_.find(title);
    Q_ASSERT(it != std::end(db_));

    QByteArray bytes = reply.readAll();
    QPixmap& pixmap  = it->second.poster;

    if (!pixmap.loadFromData(bytes))
    {
        emit posterError(title, "Invalid pixmap for poster");
        return;
    }

    emit posterReady(title);

    DEBUG("Stored poster for '%1'", title);
}

MovieDatabase* g_movies;

} // app
