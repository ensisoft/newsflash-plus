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

#include <newsflash/warnpop.h>
#include <memory>
#include "ui_rss.h"
#include "ui_rss_nzbs.h"
#include "ui_rss_settings.h"
#include "settings.h"
#include "mainwidget.h"
#include "../types.h"
#include "../rss.h"

namespace gui
{
    class mainwindow;

    struct rss_nzbs_settings {
        QString userid;
        QString apikey;
        int feedsize;
        bool enabled;
    };

    class rss_nzbs_settings_page : public settings
    {
        Q_OBJECT
    public:
        rss_nzbs_settings_page(rss_nzbs_settings& data);
       ~rss_nzbs_settings_page();

        virtual bool validate() const override;

        virtual void accept() override;
    private:
        Ui::rss_nzbs ui_;
    private:
        rss_nzbs_settings& data_;
    };

    class rss_feeds_settings_page : public settings
    {
        Q_OBJECT

    public:
        rss_feeds_settings_page(app::bitflag_t& feeds);
       ~rss_feeds_settings_page();

        virtual void accept() override;
    private:
        Ui::rss_settings ui_;
    private:
        app::bitflag_t& feeds_;
    };

    class rss : public mainwidget
    {
        Q_OBJECT

    public:
        rss(mainwindow& win, app::rss& model);
       ~rss();

        virtual void add_actions(QMenu& menu) override;
        virtual void add_actions(QToolBar& bar) override;
        virtual void add_settings(std::vector<std::unique_ptr<settings>>& pages) override;        
        virtual void apply_settings() override;

        virtual void activate(QWidget*) override;

        virtual void save(app::datastore& store) override;
        virtual void load(const app::datastore& store) override;

        virtual info information() const override;

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
        //void ready();
        void rowChanged();
        void downloadToPrevious();

    private:
        mainwindow& win_;

    private:
        app::rss& model_;        
        app::bitflag_t feeds_;

    private:
        Ui::RSS ui_;

    private:
        rss_nzbs_settings nzbs_;
    };

} // gui
