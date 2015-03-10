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
#  include "ui_search.h"
#include <newsflash/warnpop.h>
#include "mainwidget.h"
#include "../search.h"

namespace gui
{
    class Search : public MainWidget
    {
        Q_OBJECT

    public:
        Search();
       ~Search();

        virtual void addActions(QMenu& menu) override;
        virtual void addActions(QToolBar& bar) override;
        virtual void loadState(app::Settings& settings) override;
        virtual void saveState(app::Settings& settings) override;

    private slots:
        void on_actionRefresh_triggered();
        void on_actionStop_triggered();
        void on_actionSettings_triggered();
        void on_actionOpen_triggered();
        void on_actionSave_triggered();
        void on_btnSearch_clicked();
        void on_tableView_customContextMenuRequested(QPoint point);
        void tableview_selectionChanged();
        void downloadToPrevious();
    private:
        void downloadSelected(const QString& folder);

    private:
        Ui::Search ui_;
    private:
        app::Search model_;
    };
} // gui