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
#  include "ui_downloads.h"
#include <newsflash/warnpop.h>
#include <memory>
#include "mainwidget.h"
#include "../tasklist.h"
#include "../connlist.h"

class QEvent;

namespace gui
{
    // GUI module for both task and connection list.
    class Downloads : public MainWidget
    {
        Q_OBJECT

    public:
        Downloads();
       ~Downloads();

        virtual void addActions(QMenu& menu) override;
        virtual void addActions(QToolBar& bar) override;
        virtual void loadState(app::Settings& s) override;
        virtual void saveState(app::Settings& s) override;
        virtual void refresh(bool isActive) override;
        virtual void activate(QWidget*) override;

        virtual info getInformation() const override 
        { return {"downloads.html", true, true}; }

    private slots:
        void on_actionConnect_triggered();
        void on_actionPreferSSL_triggered();
        void on_actionThrottle_triggered();
        void on_actionTaskPause_triggered();
        void on_actionTaskResume_triggered();
        void on_actionTaskMoveUp_triggered();
        void on_actionTaskMoveDown_triggered();
        void on_actionTaskMoveTop_triggered();
        void on_actionTaskMoveBottom_triggered();
        void on_actionTaskDelete_triggered();
        void on_actionTaskClear_triggered();
        void on_actionTaskOpenLog_triggered();
        void on_actionConnClone_triggered();
        void on_actionConnDelete_triggered();
        void on_actionConnOpenLog_triggered();
        void on_tableTasks_customContextMenuRequested(QPoint pos);
        void on_tableConns_customContextMenuRequested(QPoint pos);
        void on_tableConns_doubleClicked(const QModelIndex&);
        void on_chkGroupSimilar_clicked(bool checked);
        void tableTasks_selectionChanged();
        void tableConns_selectionChanged();


    private:
        bool eventFilter(QObject* obj, QEvent* event);

    private:
        Ui::Downloads ui_;

    private:
        app::TaskList tasks_;
        app::ConnList conns_;
        int panels_y_pos_;

    private:
        std::size_t numDownloads_;
    };

} // gui
