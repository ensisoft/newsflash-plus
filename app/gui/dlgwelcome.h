// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
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

#include <newsflash/app/config.h>

#include <newsflash/warnpush.h>
#  include <QtGui/QDialog>
#include <newsflash/warnpop.h>
#include "ui_dlgwelcome.h"

namespace gui
{
    class DlgWelcome : public QDialog
    {
        Q_OBJECT

    public:
        DlgWelcome(QWidget* parent) : QDialog(parent)
        {
            ui_.setupUi(this);
            auto txt = ui_.welcome->text();

            txt.replace("#1", NEWSFLASH_TITLE);
            txt.replace("#2", NEWSFLASH_VERSION);
            ui_.welcome->setText(txt);
        }
       ~DlgWelcome()
        {}

        bool open_guide() const
        {
            return ui_.chkQuickStart->isChecked();
        }

    private slots:
        void on_btnLater_clicked()
        {
            reject();
        }
        void on_btnYes_clicked()
        {
            accept();
        }

    private:
        Ui::DlgWelcome ui_;

    }; 

} // gui


