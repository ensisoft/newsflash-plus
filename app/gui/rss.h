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
#  include <QTimer>
#  include "ui_rss.h"
#include <newsflash/warnpop.h>
#include <memory>

#include <newsflash/engine/bitflag.h>

#include "mainwidget.h"
#include "settings.h"
#include "dlgmovie.h"
#include "../types.h"
#include "../rss.h"
#include "../media.h"

namespace gui
{
    // RSS feeds GUI
    class RSS : public MainWidget
    {
        Q_OBJECT

    public:
        RSS();
       ~RSS();

        virtual void addActions(QMenu& menu) override;
        virtual void addActions(QToolBar& bar) override;
        virtual void activate(QWidget*) override;
        virtual void loadState(app::Settings& s) override;
        virtual bool saveState(app::Settings& s) override;
        virtual void shutdown() override;
        virtual info getInformation() const override;
        virtual SettingsWidget* getSettings() override;
        virtual void applySettings(SettingsWidget* gui) override;
        virtual void freeSettings(SettingsWidget* s);

    private:
        bool eventFilter(QObject* obj, QEvent* event);
        void downloadSelected(const QString& path);
        void refresh(bool verbose);

    private slots:
        void on_actionRefresh_triggered();
        void on_actionDownload_triggered();
        void on_actionDownloadTo_triggered();
        void on_actionSave_triggered();
        void on_actionOpen_triggered();
        void on_actionSettings_triggered();
        void on_actionStop_triggered();
        void on_actionBrowse_triggered();
        void on_tableView_customContextMenuRequested(QPoint point);
        void on_tableView_doubleClicked(const QModelIndex&);

    private slots:
        void rowChanged();
        void downloadToPrevious();
        void popupDetails();

    private:
        Ui::RSS ui_;
    private:
        app::rss model_;        
    private:
        newsflash::bitflag<app::media> streams_;
        bool enable_nzbs_;
        bool enable_womble_;
        QString nzbs_userid_;
        QString nzbs_apikey_;
        QTimer  popup_;
    private:
        std::unique_ptr<DlgMovie> movie_;
    };

} // gui
