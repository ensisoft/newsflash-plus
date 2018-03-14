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

#pragma once

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QtGui/QPixmap>
#  include <QObject>
#  include <QString>
#include "newsflash/warnpop.h"
#include <map>

class QNetworkReply;

namespace app
{
    class MovieDatabase : public QObject
    {
        Q_OBJECT

    public:
        struct Movie {
            QString title;
            QString genre;
            QString plot;
            QString director;
            QString language;
            QString country;
            QString runtime;
            QString actors;
            QString imdbid;
            QString rating;
            QString year;
            QPixmap poster;
        };


        MovieDatabase();
       ~MovieDatabase();

        bool beginLookup(const QString& title, const QString& releaseYear = "");
        void abortLookup(const QString& title);

        bool testLookup(const QString& testKey, const QString& testTitle);

        void setApikey(const QString& apikey)
        { mApikey = apikey; }

        QString getApikey() const
        { return mApikey; }

        bool hasApikey() const
        { return !mApikey.isEmpty(); }

        const Movie* getMovie(const QString& title, const QString& releaseYear = "") const;

    signals:
        void lookupReady(const QString& title);
        void lookupError(const QString& title, const QString& error);
        void posterReady(const QString& title);
        void posterError(const QString& title, const QString& error);

    private:
        void onLookupFinished(QNetworkReply& reply, const QString& title);
        void onPosterFinished(QNetworkReply& reply, const QString& title);

    private:
        std::map<QString, Movie> mMovieMap;
    private:
        QString mApikey;
    };

    extern MovieDatabase* g_movies;

} // app
