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
    mUI.setupUi(this);
    mUI.btnDownload->setEnabled(false);
    mUI.btnCredentials->setVisible(false);
    mUI.lblMessage->setVisible(false);

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

    mLoader.setFileName(":/resource/ajax-loader.gif");
}

DlgMovie::~DlgMovie()
{}

void DlgMovie::lookupMovie(const QString& movieTitle, const QString& guid)
{
    mTitle = movieTitle;
    mGuid  = guid;

    const auto* movie = app::g_movies->getMovie(movieTitle);
    if (!movie)
    {
        if (app::g_movies->beginLookup(movieTitle))
        {
            mLoader.start();
            mUI.lblPoster->setMovie(&mLoader);
            mUI.lblPoster->setFixedSize(100, 100); // todo: fix this, the label shrinks and doesn't properly update its geometry
            mUI.lblPoster->updateGeometry();
            mUI.textEdit->clear();
            mUI.btnCredentials->setVisible(false);
            mUI.lblMessage->setVisible(false);
        }
        else
        {
            mUI.lblPoster->setMovie(nullptr);
            mUI.lblPoster->setVisible(false);
            mUI.lblMessage->setVisible(true);
            mUI.btnCredentials->setVisible(true);

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
    mTitle = "Terminator 2";

    mUI.btnDownload->setVisible(false);

    if (app::g_movies->testLookup(apikey, mTitle))
    {
        mLoader.start();
        mUI.lblPoster->setMovie(&mLoader);
        mUI.lblPoster->setFixedSize(100, 100);
        mUI.lblPoster->updateGeometry();
        mUI.textEdit->clear();
        mUI.btnCredentials->setVisible(false);
        mUI.lblMessage->setVisible(false);
    }
    else
    {
        mUI.lblPoster->setMovie(nullptr);
        mUI.lblPoster->setVisible(false);
        mUI.lblMessage->setText("An error occurred :(");
        mUI.lblMessage->setVisible(true);
    }
    reposition();
    show();
    activateWindow();
    setWindowTitle(QString("%1 (Click to Close Window)").arg(mTitle));
}

void DlgMovie::lookupSeries(const QString& seriesTitle, const QString& guid)
{
    // this is a simple forward for now. maybe in the future
    // we want Season and Episode here as well.
    lookupMovie(seriesTitle, guid);
}

void DlgMovie::on_btnCredentials_clicked()
{
    g_win->showSetting("Omdb");
    if (app::g_movies->hasApiKey())
    {
        lookupMovie(mTitle, mGuid);
    }
}

void DlgMovie::on_btnDownload_clicked()
{
    emit startMovieDownload(mGuid);
    close();
}

void DlgMovie::lookupReady(const QString& title)
{
    if (title != mTitle)
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
    mUI.textEdit->setText(ss.join(""));
    mUI.btnDownload->setEnabled(true);
}

void DlgMovie::posterReady(const QString& title)
{
    if (mTitle != title)
        return;

    const auto* movie = app::g_movies->getMovie(title);

    QPixmap pix = movie->poster;
    if (pix.height() > 600)
        pix = pix.scaledToHeight(600);

    mUI.lblPoster->setFixedSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    mUI.lblPoster->setPixmap(pix);
    mUI.lblPoster->updateGeometry();
    mUI.lblPoster->setVisible(true);

    QTimer::singleShot(0, this, SLOT(reposition()));
}

void DlgMovie::lookupError(const QString& title, const QString& errorStr)
{
    mUI.lblPoster->setMovie(nullptr);
    mUI.lblPoster->setText(errorStr);
    mUI.btnDownload->setEnabled(false);
}

void DlgMovie::posterError(const QString& title, const QString& errorStr)
{
    mUI.lblPoster->setMovie(nullptr);
    mUI.lblPoster->setText("Poster not available");
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
