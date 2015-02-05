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

#include "dlgmovie.h"
#include "../omdb.h"

namespace gui
{

DlgMovie::DlgMovie(QWidget* parent) : QDialog(parent)
{
    ui_.setupUi(this);
    setMouseTracking(true);

    QObject::connect(app::g_movies, SIGNAL(lookupReady(const QString&)),
        this, SLOT(lookupReady(const QString&)));
    QObject::connect(app::g_movies, SIGNAL(posterReady(const QString&)),
        this, SLOT(posterReady(const QString&)));

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
        ui_.edtText->clear();
    }
    else
    {

    }
    show();
}

void DlgMovie::lookupReady(const QString& title)
{

}

void DlgMovie::posterReady(const QString& title)
{}

bool DlgMovie::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseMove)
    {
        //close();
        return true;
    }
    return QDialog::eventFilter(obj, event);
}

} // gui
