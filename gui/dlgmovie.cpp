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

#include "newsflash/config.h"
#include "newsflash/warnpush.h"
#  include <QTimer>
#include "newsflash/warnpop.h"
#include "dlgmovie.h"
#include "mainwindow.h"
#include "app/omdb.h"

namespace gui
{

DlgMovie::DlgMovie(QWidget* parent) : QDialog(parent)
{
    ui_.setupUi(this);
    ui_.btnCredentials->setVisible(false);
    ui_.lblMessage->setVisible(false);

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

void DlgMovie::lookupMovie(const QString& movieTitle)
{
    title_ = movieTitle;

    const auto* movie = app::g_movies->getMovie(movieTitle);
    if (!movie)
    {
        if (app::g_movies->beginLookup(movieTitle))
        {
            loader_.start();
            ui_.lblPoster->setMovie(&loader_);
            ui_.lblPoster->setFixedSize(100, 100); // todo: fix this, the label shrinks and doesn't properly update its geometry
            ui_.lblPoster->updateGeometry();
            ui_.textEdit->clear();
            ui_.btnCredentials->setVisible(false);
            ui_.lblMessage->setVisible(false);
        }
        else
        {
            ui_.lblPoster->setMovie(nullptr);
            ui_.lblPoster->setVisible(false);
            ui_.lblMessage->setVisible(true);
            ui_.btnCredentials->setVisible(true);

        }
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

void DlgMovie::testLookupMovie(const QString& apikey)
{
    title_ = "Terminator 2";

    if (app::g_movies->testLookup(apikey, title_))
    {
        loader_.start();
        ui_.lblPoster->setMovie(&loader_);
        ui_.lblPoster->setFixedSize(100, 100);
        ui_.lblPoster->updateGeometry();
        ui_.textEdit->clear();
        ui_.btnCredentials->setVisible(false);
        ui_.lblMessage->setVisible(false);
    }
    else
    {
        ui_.lblPoster->setMovie(nullptr);
        ui_.lblPoster->setVisible(false);
        ui_.lblMessage->setText("An error occurred :(");
        ui_.lblMessage->setVisible(true);
    }
    reposition();
    show();
    activateWindow();
    setWindowTitle(QString("%1 (Click to Close Window)").arg(title_));
}

void DlgMovie::lookupSeries(const QString& seriesTitle)
{
    // this is a simple forward for now. maybe in the future
    // we want Season and Episode here as well.
    lookupMovie(seriesTitle);
}

void DlgMovie::on_btnCredentials_clicked()
{
    g_win->showSetting("Omdb");
    if (app::g_movies->hasApiKey())
    {
        lookupMovie(title_);
    }
}

void DlgMovie::lookupReady(const QString& title)
{
    if (title != title_)
        return;

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
    if (title_ != title)
        return;

    const auto* movie = app::g_movies->getMovie(title);

    QPixmap pix = movie->poster;
    if (pix.height() > 600)
        pix = pix.scaledToHeight(600);

    ui_.lblPoster->setFixedSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    ui_.lblPoster->setPixmap(pix);
    ui_.lblPoster->updateGeometry();
    ui_.lblPoster->setVisible(true);

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
