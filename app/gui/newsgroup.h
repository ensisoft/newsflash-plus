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
#  include "ui_newsgroup.h"
#include <newsflash/warnpop.h>
#include "mainwidget.h"
#include "finder.h"
#include "../newsgroup.h"

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
        virtual bool canClose() const override;
        virtual Finder* getFinder() override;

        // Finder
        virtual bool isMatch(const QString& str, std::size_t index, bool caseSensitive) override;
        virtual bool isMatch(const QRegExp& reg, std::size_t index) override;
        virtual std::size_t numItems() const override;
        virtual std::size_t curItem() const override;
        virtual void setFound(std::size_t index) override;

        void load();

    private slots:
        void on_actionRefresh_triggered();
        void on_actionFilter_triggered();
        void on_actionStop_triggered();
        void on_actionDownload_triggered();
        void on_btnLoadMore_clicked();
        void on_tableView_customContextMenuRequested(QPoint p);
        void downloadToPrevious();
        void selectionChanged(const QItemSelection& sel, const QItemSelection& deSel);
        void modelBegReset();        
        void modelEndReset();
        void newHeaderInfoAvailable(const QString& group, quint64 numLocal, quint64 numRemote);
        void updateCompleted(const app::HeaderInfo& info);
    private:
        void downloadSelected(QString folder);

    private:
        Ui::NewsGroup ui_;
    private:
        app::NewsGroup model_;
    private:
        quint32 account_;
        QString path_;
        QString name_;
        quint32 blockIndex_;
    private:
        bool loadingState_;

    };
} // gui
