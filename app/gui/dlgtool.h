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
#  include "ui_dlgtool.h"
#include <newsflash/warnpop.h>

#include "../tools.h"

namespace gui
{
    class DlgTool : public QDialog
    {
        Q_OBJECT

    public:
        DlgTool(QWidget* parent, app::tools::tool& tool);
       ~DlgTool();

    private slots:
        void on_btnBrowse_clicked();
        void on_btnAccept_clicked();
        void on_btnCancel_clicked();
        void on_btnDefault_clicked();
        void on_btnReset_clicked();

    private:
        bool eventFilter(QObject* receiver, QEvent* event);

    private:
        Ui::DlgTool ui_;
    private:
        app::tools::tool& tool_;
    };
} // gui