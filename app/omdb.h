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

#pragma once

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QtGui/QPixmap>
#  include <QObject>
#  include <QString>
#include <newsflash/warnpop.h>
#include <map>
#include "netman.h"

class QString;

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
            QPixmap poster;
            float rating;
            int year;
        };


        MovieDatabase();
       ~MovieDatabase();

        void beginLookup(const QString& title);
        void abortLookup(const QString& title);

        const Movie* getMovie(const QString& title) const;

    signals:
        void lookupReady(const QString& title);
        void lookupError(const QString& title, const QString& error);
        void posterReady(const QString& title);
        void posterError(const QString& title, const QString& error);

    private:
        void onLookupFinished(QNetworkReply& reply, const QString& title);
        void onPosterFinished(QNetworkReply& reply, const QString& title);
        
    private:
        NetworkManager::Context net_;

        std::map<QString, Movie> db_;
    };

    extern MovieDatabase* g_movies;

} // app