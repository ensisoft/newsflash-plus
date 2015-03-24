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
#  include "ui_groups.h"
#include <newsflash/warnpop.h>

#include "mainwidget.h"
#include "../newslist.h"

namespace gui
{
    // news group list GUI
    class Groups : public MainWidget
    {
        Q_OBJECT

    public:
        Groups();
       ~Groups();

        virtual void addActions(QMenu& menu) override;
        virtual void addActions(QToolBar& bar) override;
        virtual void activate(QWidget*) override;
        virtual void loadState(app::Settings& settings) override;
        virtual void saveState(app::Settings& settings) override;

        virtual info getInformation() const override;

    private slots:
        void on_actionBrowse_triggered();
        void on_actionRefresh_triggered();
        void on_actionFavorite_triggered();
        void on_actionUnfavorite_triggered();
        void on_cmbAccounts_currentIndexChanged();
        void on_tableGroups_customContextMenuRequested(QPoint point);
        void on_chkFavorites_clicked(bool state);

        void accountsUpdated();
        void progressUpdated(quint32 acc, quint32 maxValue, quint32 curValue);
        void loadComplete(quint32 acc);
        void makeComplete(quint32 acc);

    private:
        void resort();

    private:
        Ui::Groups ui_;
    private:
        app::NewsList model_;
        quint32 curAccount_;
    };

} // gui
