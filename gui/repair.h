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
#  include "ui_repair.h"
#include "newsflash/warnpop.h"
#include "mainwidget.h"

namespace app {
    class Repairer;
    struct Archive;
} // app

namespace gui
{
    class Repair : public MainWidget
    {
        Q_OBJECT

    public:
        Repair(app::Repairer& repairer);
       ~Repair();

        virtual void addActions(QToolBar& bar) override;
        virtual void addActions(QMenu& menu) override;
        virtual void activate(QWidget*) override;
        virtual void deactivate() override;
        virtual void refresh(bool isActive) override;
        virtual void loadState(app::Settings& settings) override;
        virtual void saveState(app::Settings& settings) override;
        virtual void shutdown() override;

        virtual info getInformation() const override
        { return {"repairs.html", true}; }

        virtual bool dropFile(const QString& file) override;

        void setRepairEnabled(bool onOff);

        std::size_t numNewRepairs() const
        { return numRepairs_; }

    private slots:
        void on_repairList_customContextMenuRequested(QPoint);
        void repairList_selectionChanged();
        void on_actionRepair_triggered();
        void on_actionDelete_triggered();
        void on_actionMoveUp_triggered();
        void on_actionMoveDown_triggered();
        void on_actionTop_triggered();
        void on_actionBottom_triggered();
        void on_actionClear_triggered();
        void on_actionOpenLog_triggered();
        void on_actionStop_triggered();
        void on_actionOpenFolder_triggered();
        void on_actionDetails_triggered();
        void on_chkWriteLogs_stateChanged(int);
        void on_chkPurgePars_stateChanged(int);
        void repairStart(const app::Archive& arc);
        void repairReady(const app::Archive& arc);
        void repairProgress(const QString& step, int done);
        void scanProgress(const QString& file, int val);

    private:
        enum class TaskDirection {
            Up, Down, Top, Bottom
        };
        void moveTasks(TaskDirection dir);

    private:
        Ui::Repair ui_;
    private:
        app::Repairer& model_;
    private:
        std::size_t numRepairs_;


    };
} // gui

