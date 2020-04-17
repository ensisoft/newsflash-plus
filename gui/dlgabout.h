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
#  include <QDialog>
#  include "ui_dlgabout.h"
#include "newsflash/warnpop.h"

namespace gui
{
    class DlgAbout : public QDialog
    {
        Q_OBJECT

    public:
        DlgAbout(QWidget* parent) : QDialog(parent)
        {
            ui_.setupUi(this);
            ui_.lblTitle->setText(QString("%1 %2 %3").arg(NEWSFLASH_TITLE).arg(NEWSFLASH_VERSION).arg(NEWSFLASH_ARCH));
            ui_.lblCopyright->setText(QString::fromUtf8(NEWSFLASH_COPYRIGHT));
        }
       ~DlgAbout()
        {}

    private:
        Ui::DlgAbout ui_;
    };
} // gui