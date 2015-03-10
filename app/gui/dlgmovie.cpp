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

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QTimer>
#include <newsflash/warnpop.h>
#include "dlgmovie.h"
#include "../omdb.h"

namespace gui
{

DlgMovie::DlgMovie(QWidget* parent) : QDialog(parent)
{
    ui_.setupUi(this);

    setMouseTracking(true);
    //setWindowFlags(Qt::Popup /* | Qt::FramelessWindowHint */);    

    QObject::connect(app::g_movies, SIGNAL(lookupReady(const QString&)),
        this, SLOT(lookupReady(const QString&)));
    QObject::connect(app::g_movies, SIGNAL(posterReady(const QString&)),
        this, SLOT(posterReady(const QString&)));

    QObject::connect(app::g_movies, SIGNAL(lookupError(const QString&, const QString&)),
        this, SLOT(lookupError(const QString&, const QString&)));
    QObject::connect(app::g_movies, SIGNAL(posterError(const QString&, const QString&)),
        this, SLOT(posterError(const QString&, const QString&)));

    loader_.setFileName(":/resource/ajax-loader.gif");
}

DlgMovie::~DlgMovie()
{}

void DlgMovie::lookup(const QString& movieTitle)
{
    const auto* movie = app::g_movies->getMovie(movieTitle);
    if (!movie)
    {
        app::g_movies->beginLookup(movieTitle);
        loader_.start();
        ui_.lblPoster->setMovie(&loader_);
        ui_.lblPoster->setFixedSize(100, 100); // todo: fix this, the label shrinks and doesn't properly update its geometry
        ui_.lblPoster->updateGeometry();
        ui_.textEdit->clear();
    }
    else
    {
        lookupReady(movieTitle);
        posterReady(movieTitle);
    }
    reposition();    
    show();
    activateWindow();
    setWindowTitle(QString("%1 (Click to Close Window)").arg(movieTitle));
}

void DlgMovie::lookupReady(const QString& title)
{
    const auto* movie = app::g_movies->getMovie(title);

    QStringList ss;
    ss << "Title: " << movie->title << "<br>";
    ss << "Genre: " << movie->genre << "<br>";
    ss << "Director: " << movie->director << "<br>";
    ss << "Actors: " << movie->actors << "<br>";
    ss << "Language: " << movie->language << "<br>";
    ss << "Country: " << movie->country << ", " << QString::number(movie->year) << "<br>";
    ss << "Runtime: " << movie->runtime << "<br>";
    ss << "IMDb Score: " << QString::number(movie->rating) << "<br>";
    ss << "<br>";
    ss << movie->plot;
    ss << QString("<br><a href=http://www.imdb.com/title/%1>more ...</a>").arg(movie->imdbid);
    ui_.textEdit->setText(ss.join(""));
}

void DlgMovie::posterReady(const QString& title)
{
    const auto* movie = app::g_movies->getMovie(title);

    QPixmap pix = movie->poster;
    if (pix.height() > 600)
        pix = pix.scaledToHeight(600);

    ui_.lblPoster->setFixedSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    ui_.lblPoster->setPixmap(pix);

    QTimer::singleShot(0, this, SLOT(reposition()));
}

void DlgMovie::lookupError(const QString& title, const QString& errorStr)
{
    ui_.lblPoster->setMovie(nullptr);
    ui_.lblPoster->setText(errorStr);
}

void DlgMovie::posterError(const QString& title, const QString& errorStr)
{
    ui_.lblPoster->setMovie(nullptr);
    ui_.lblPoster->setText("Poster not available");
}

void DlgMovie::reposition()
{
    updateGeometry();

    const auto* w = parentWidget();

    // find the application "top level, i.e. the MainWindow
    while (w)
    {
        const auto* p = w->parentWidget();
        if (p == nullptr)
            break;
        w = p;
    }

    Q_ASSERT(w);

    const auto mainGeom = w->frameGeometry();
    const auto mainWidth  = mainGeom.width();
    const auto mainHeight = mainGeom.height();

    const auto myGeom   = frameGeometry();
    const auto myWidth  = myGeom.width();
    const auto myHeight = myGeom.height();

    QPoint position(mainGeom.left() + (mainWidth - myWidth) / 2,
        mainGeom.top() + (mainHeight - myHeight) / 2);

    move(position);
}

void DlgMovie::mousePressEvent(QMouseEvent* mickey)
{
    close();
}

} // gui
