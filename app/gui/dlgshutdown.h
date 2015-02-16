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

#include <newsflash/warnpush.h>
#  include <QtGui/QDialog>
#  include <QtGui/QCloseEvent>
#  include <QtGui/QKeyEvent>
#  include "ui_dlgshutdown.h"
#include <newsflash/warnpop.h>

#include "../debug.h"

namespace gui
{
    class DlgShutdown : public QDialog
    {
        Q_OBJECT
    public:
        DlgShutdown(QWidget* parent) : QDialog(parent)
        {
            ui_.setupUi(this);
            ui_.progressBar->setValue(0);
            ui_.progressBar->setRange(0, 0);
            ui_.lblBye->setText(tr("Thank you for using %1.").arg(NEWSFLASH_TITLE));

            const auto flags = windowFlags();

            setWindowFlags(flags & ~Qt::WindowCloseButtonHint);
        }
       ~DlgShutdown()
        {}

        void setText(const QString& s)
        {
            ui_.lblAction->setText(s);
        }
    private:
        void closeEvent(QCloseEvent* event)
        {
            DEBUG("DlgShutdown close event");

            // just in case the window has a little X in the title bar
            // we want to disallow the window to be closed by the user.
            // this window should only be closed when the application
            // says so.
            event->ignore();
        }
        void keyPressEvent(QKeyEvent* event)
        {
            // we also eat Esc keys to make sure that the dialog cannot be closed on Esc.
            if (event->key() == Qt::Key_Escape)
                return;
            QDialog::keyPressEvent(event);
        }
    private:
        Ui::DlgShutdown ui_;
    };
} // gui