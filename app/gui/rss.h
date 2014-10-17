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
#  include "ui_rss.h"
#include <newsflash/warnpop.h>
#include <memory>

#include <newsflash/engine/bitflag.h>

#include "mainwidget.h"
#include "settings.h"
#include "../types.h"
#include "../rss.h"
#include "../media.h"

namespace gui
{
    class rss : public mainwidget
    {
        Q_OBJECT

    public:
        rss();
       ~rss();

        virtual void add_actions(QMenu& menu) override;
        virtual void add_actions(QToolBar& bar) override;
        virtual void activate(QWidget*) override;
        virtual void loadstate(app::settings& s) override;
        virtual bool savestate(app::settings& s) override;
        virtual info information() const override;
        virtual gui::settings* get_settings(app::settings& s) override;
        virtual void apply_settings(gui::settings* gui, app::settings& backend) override;
        virtual void free_settings(gui::settings* s);

    private:
        void download_selected(const QString& path);
        //void save_selected(const QString& path);
        //void open_selected()
        void ready();

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

    private slots:
        void rowChanged();
        void downloadToPrevious();

    private:
        Ui::RSS ui_;
    private:
        app::rss model_;        
    private:
        newsflash::bitflag<app::media> streams_;
        bool enable_nzbs_;
        bool enable_womble_;
    };

} // gui
