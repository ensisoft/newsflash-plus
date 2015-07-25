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

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QtGui/QDialog>
#  include <QtGui/QMovie>
#  include <QtGui/QPixmap>
#  include "ui_movie.h"
#include <newsflash/warnpop.h>

namespace gui
{
    // Movie details dialog. Also works for TV Series.
    class DlgMovie : public QDialog
    {
        Q_OBJECT

    public:
        DlgMovie(QWidget* parent);
       ~DlgMovie();

        // show the dialog and initiate lookup for the Movie 
        void lookupMovie(const QString& movieTitle);

        // show the dialog and initite lookup for the TV Series
        void lookupSeries(const QString& seriesTitle);

    private slots:
        void lookupReady(const QString& title);
        void posterReady(const QString& title);
        void lookupError(const QString& title, const QString& errorStr);
        void posterError(const QString& title, const QString& errorStr);
        void reposition();
    private:
        void mousePressEvent(QMouseEvent* mickey);

    private:
        Ui::Movie ui_;
    private:
        QMovie  loader_;
        QPixmap poster_;
        QString title_;
    };
}