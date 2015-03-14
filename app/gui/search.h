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
#  include <QStringList>
#  include <QTimer>
#  include "ui_search.h"
#include <newsflash/warnpop.h>
#include <memory>
#include "mainwidget.h"
#include "../search.h"

namespace gui
{
    class SearchModule;
    class DlgMovie;

    class Search : public MainWidget
    {
        Q_OBJECT

    public:
        Search(SearchModule& module);
       ~Search();

        virtual void addActions(QMenu& menu) override;
        virtual void addActions(QToolBar& bar) override;
        virtual void loadState(app::Settings& settings) override;
        virtual void saveState(app::Settings& settings) override;

    public slots:
        void updateBackendList(const QStringList& names);

    private slots:
        void on_actionRefresh_triggered();
        void on_actionStop_triggered();
        void on_actionSettings_triggered();
        void on_actionOpen_triggered();
        void on_actionSave_triggered();
        void on_actionDownload_triggered();
        void on_actionDownloadTo_triggered();
        void on_actionBrowse_triggered();
        void on_btnSearch_clicked();
        void on_btnSearchMore_clicked();
        void on_tableView_customContextMenuRequested(QPoint point);
        void on_editSearch_returnPressed();
        void tableview_selectionChanged();
        void downloadToPrevious();
        void popupDetails();
    private:
        virtual bool eventFilter(QObject* obj, QEvent* event) override;
        void downloadSelected(const QString& folder);
        void beginSearch(quint32 queryOffset, quint32 querySize);

    private:
        Ui::Search ui_;
    private:
        app::Search model_;
        SearchModule& module_;
        std::unique_ptr<DlgMovie> movie_;
    private:
        QTimer popup_;
        quint32 offset_;
    };
} // gui