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
#  include <QTimer>
#  include "ui_rss.h"
#include "newsflash/warnpop.h"

#include <memory>

#include "engine/bitflag.h"
#include "mainwidget.h"
#include "settings.h"
#include "dlgmovie.h"
#include "finder.h"
#include "app/types.h"
#include "app/rssreader.h"
#include "app/media.h"

namespace gui
{
    // RSS feeds GUI
    class RSS : public MainWidget, public Finder
    {
        Q_OBJECT

    public:
        RSS();
       ~RSS();

        // MainWidget implementation
        virtual void addActions(QMenu& menu) override;
        virtual void addActions(QToolBar& bar) override;
        virtual void activate(QWidget*) override;
        virtual void loadState(app::Settings& s) override;
        virtual void saveState(app::Settings& s) override;
        virtual void shutdown() override;
        virtual info getInformation() const override;
        virtual SettingsWidget* getSettings() override;
        virtual void applySettings(SettingsWidget* gui) override;
        virtual void freeSettings(SettingsWidget* s);
        virtual Finder* getFinder() override;
        virtual void firstLaunch() override;

        // Finder implementation
        virtual bool isMatch(const QString& str, std::size_t index, bool caseSensitive) override;
        virtual bool isMatch(const QRegExp& regex, std::size_t index) override;
        virtual std::size_t numItems() const override;
        virtual std::size_t curItem() const override;
        virtual void setFound(std::size_t index) override;

    private:
        bool eventFilter(QObject* obj, QEvent* event);
        void downloadSelected(const QString& path);
        void refreshStreams(bool verbose);

    private slots:
        void on_actionRefresh_triggered();
        void on_actionDownload_triggered();
        void on_actionDownloadTo_triggered();
        void on_actionSave_triggered();
        void on_actionOpen_triggered();
        void on_actionSettings_triggered();
        void on_actionStop_triggered();
        void on_actionBrowse_triggered();
        void on_actionInformation_triggered();
        void on_tableView_customContextMenuRequested(QPoint point);
        void on_tableView_doubleClicked(const QModelIndex&);

    private slots:
        void rowChanged();
        void downloadToPrevious();
        void popupDetails();

    private:
        Ui::RSS ui_;
    private:
        app::RSSReader model_;        
    private:
        newsflash::bitflag<app::MediaType,
            std::uint64_t> streams_;
        bool enable_nzbs_;
        bool enable_womble_;
        bool show_popup_hint_;
        QString nzbs_userid_;
        QString nzbs_apikey_;
        QTimer  popup_;
    private:
        std::unique_ptr<DlgMovie> movie_;
    };

} // gui
