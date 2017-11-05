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
#  include "ui_newsgroup.h"
#include "newsflash/warnpop.h"
#include "mainwidget.h"
#include "finder.h"
#include "dlgfilter.h"
#include "app/newsgroup.h"

namespace app {
    struct HeaderInfo;
} // app

namespace gui
{
    class NewsGroup : public MainWidget, public Finder
    {
        Q_OBJECT

    public:
        NewsGroup(quint32 account, QString path, QString group);
       ~NewsGroup();

        virtual void addActions(QMenu& menu) override;
        virtual void addActions(QToolBar& bar) override;
        virtual void loadState(app::Settings& settings) override;
        virtual void saveState(app::Settings& settings) override;
        virtual info getInformation() const override;
        virtual Finder* getFinder() override;

        // Finder
        virtual bool isMatch(const QString& str, std::size_t index, bool caseSensitive) override;
        virtual bool isMatch(const QRegExp& reg, std::size_t index) override;
        virtual std::size_t numItems() const override;
        virtual std::size_t curItem() const override;
        virtual void setFound(std::size_t index) override;

        void load();

        static
        void deleteData(quint32 account, QString path, QString group);

    private slots:
        void on_actionShowNone_changed();
        void on_actionShowAudio_changed();
        void on_actionShowVideo_changed();
        void on_actionShowImage_changed();
        void on_actionShowText_changed();
        void on_actionShowArchive_changed();
        void on_actionShowParity_changed();
        void on_actionShowDocument_changed();
        void on_actionShowOther_changed();
        void on_actionShowBroken_changed();
        void on_actionShowDeleted_changed();
        void on_actionDelete_triggered();
        void on_actionHeaders_triggered();
        void on_actionRefresh_triggered();
        void on_actionFilter_triggered();
        void on_actionStop_triggered();
        void on_actionDownload_triggered();
        void on_actionBrowse_triggered();
        void on_actionBookmark_triggered();
        void on_actionBookmarkPrev_triggered();
        void on_actionBookmarkNext_triggered();
        void on_btnLoadMore_clicked();
        void on_tableView_customContextMenuRequested(QPoint p);
        void on_tableView_doubleClicked(const QModelIndex&);
        void downloadToPrevious();
        void selectionChanged(const QItemSelection& sel, const QItemSelection& deSel);
        void modelBegReset();
        void modelEndReset();
        void modelLayoutChanged();
        void newHeaderInfoAvailable(const app::HeaderUpdateInfo& info);
        void updateCompleted(const app::HeaderInfo& info);
    private:
        bool eventFilter(QObject* recv, QEvent* event);
        void downloadSelected(const QString& folder);

    private:
        Ui::NewsGroup ui_;
    private:
        app::NewsGroup model_;
    private:
        quint32 account_;
        QString path_;
        QString name_;
    private:
        DlgFilter::Params filter_;
    private:
        QModelIndexList selected_rows_;
        QModelIndex     current_index_;
    };
} // gui
