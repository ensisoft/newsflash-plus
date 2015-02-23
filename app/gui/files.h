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
#  include "ui_files.h"
#include <newsflash/warnpop.h>

#include "mainwidget.h"
#include "../files.h"

namespace gui
{
    // downloaded files GUI
    class Files : public MainWidget
    {
        Q_OBJECT

    public:
        Files();
       ~Files();

        virtual void addActions(QMenu& menu) override;
        virtual void addActions(QToolBar& bar) override;
        virtual void loadState(app::Settings& s) override;
        virtual void saveState(app::Settings& s) override;
        virtual void shutdown() override;
        virtual void refresh(bool isActive) override;        
        virtual void activate(QWidget*) override;

        virtual info getInformation() const override
        {
            return {"files.html", true, true};
        }
    private slots:
        void on_actionOpenFile_triggered();
        void on_actionOpenFileWith_triggered();
        void on_actionClear_triggered();
        void on_actionOpenFolder_triggered();        
        void on_actionFind_triggered();
        void on_actionFindNext_triggered();
        void on_actionFindPrev_triggered();
        void on_actionDelete_triggered();
        void on_tableFiles_customContextMenuRequested(QPoint point);
        void on_tableFiles_doubleClicked();
        void on_btnCloseFind_clicked();
        void on_editFind_returnPressed();
        void on_chkKeepSorted_clicked();
        void invokeTool();
        void toolsUpdated();

    private:
        void findNext(bool forward);

    private:
        Ui::Files ui_;
    private:
        app::Files model_;
    private:
        std::size_t numFiles_;
    };
} // gui