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
#  include <QtGui/QWidget>
#  include <QObject>
#  include <QRegExp>
#  include "ui_findwidget.h"
#include <newsflash/warnpop.h>

namespace gui
{
    class Finder;

    // reusable find widget. provides all the UI
    // operations to drive a Finder object
    class FindWidget : public QWidget
    {
        Q_OBJECT

    public:
        FindWidget(QWidget* parent, Finder& finder);
       ~FindWidget();

        void find();
        void findNext();
        void findPrev();

    private slots:
        void on_btnPrev_clicked();
        void on_btnNext_clicked();
        void on_btnQuit_clicked();
        void on_editFind_returnPressed();
        void on_editFind_textEdited();

    private:
        void doFind(bool forward);

    private:
        Ui::Find ui_;

    private:
        Finder& finder_;
    private:
        QRegExp regexp_; // cache here
    };
} // gui