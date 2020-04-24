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
#  include "ui_newslist.h"
#include "newsflash/warnpop.h"
#include "mainwidget.h"
#include "finder.h"
#include "app/newslist.h"

namespace gui
{
    // news group list GUI
    class NewsList : public MainWidget, public Finder
    {
        Q_OBJECT

    public:
        NewsList();
       ~NewsList();

        virtual void addActions(QMenu& menu) override;
        virtual void addActions(QToolBar& bar) override;
        virtual void activate(QWidget*) override;
        virtual void loadState(app::Settings& settings) override;
        virtual void saveState(app::Settings& settings) override;
        virtual info getInformation() const override;
        virtual Finder* getFinder();

        // finder implementation
        virtual bool isMatch(const QString& str, std::size_t index, bool caseSensitive) override;
        virtual bool isMatch(const QRegExp& regex, std::size_t index) override;
        virtual std::size_t numItems() const override;
        virtual std::size_t curItem() const override;
        virtual void setFound(std::size_t index) override;

    private slots:
        void on_actionBrowse_triggered();
        void on_actionRefresh_triggered();
        void on_actionFavorite_triggered();
        void on_actionDeleteData_triggered();
        void on_actionStop_triggered();
        void on_cmbAccounts_currentIndexChanged(int current);
        void on_tableGroups_customContextMenuRequested(QPoint point);
        void on_tableGroups_doubleClicked(const QModelIndex& index);
        void    tableGroups_selectionChanged();
        void on_editFilter_returnPressed();
        void on_editFilter_textChanged();
        void on_chkShowEmpty_clicked();
        void on_chkShowText_clicked();
        void on_chkMusic_clicked();
        void on_chkMovies_clicked();
        void on_chkTV_clicked();
        void on_chkGames_clicked();
        void on_chkApps_clicked();
        void on_chkImages_clicked();
        void on_chkOther_clicked();
        void on_chkAdult_clicked();

        void accountsUpdated();
        void progressUpdated(quint32 acc, quint32 maxValue, quint32 curValue);
        void loadComplete(quint32 acc);
        void makeComplete(quint32 acc);
        void listUpdate(quint32 acc);

    private:
        void resort();
        void filter();

    private:
        Ui::NewsList m_ui;
    private:
        app::NewsList m_model;
    private:
        quint32 m_curAccount;
    };

} // gui
