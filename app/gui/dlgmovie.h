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
#  include <QtGui/QDialog>
#  include <QtGui/QMovie>
#  include <QtGui/QPixmap>
#  include "ui_movie.h"
#include <newsflash/warnpop.h>

namespace gui
{
    class DlgMovie : public QDialog
    {
        Q_OBJECT

    public:
        DlgMovie(QWidget* parent);
       ~DlgMovie();

        void lookup(const QString& movieTitle);

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
    };
}