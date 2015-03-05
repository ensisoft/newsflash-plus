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
#  include "ui_unpack.h"
#include <newsflash/warnpop.h>
#include "mainwidget.h"

namespace app {
    class Unpacker;
    struct Archive;
}

namespace gui
{
    class Unpack : public MainWidget
    {
        Q_OBJECT
        
    public:
        Unpack(app::Unpacker& unpacker);
       ~Unpack();

        virtual void addActions(QToolBar& bar) override;
        virtual void addActions(QMenu& menu) override;
        virtual void activate(QWidget*) override;
        virtual void deactivate() override;
        virtual void refresh(bool isActive) override;
        virtual void loadState(app::Settings& settings) override;
        virtual void saveState(app::Settings& settings) override;
        virtual void shutdown() override;

        virtual info getInformation() const override
        { return {"archives.html", true}; }

        void setUnpackEnabled(bool onOff);
        
        std::size_t numNewUnpacks() const 
        { return numUnpacks_; }

    private slots:
        void on_unpackList_customContextMenuRequested(QPoint);
        void unpackList_selectionChanged();
        void on_actionUnpack_triggered();    
        void on_actionStop_triggered();
        void on_actionOpenFolder_triggered();
        void on_actionTop_triggered();
        void on_actionMoveUp_triggered();
        void on_actionMoveDown_triggered();
        void on_actionBottom_triggered();
        void on_actionOpenLog_triggered();
        void on_actionClear_triggered();
        void on_actionDelete_triggered();
        void on_chkWriteLog_stateChanged(int);
        void on_chkOverwriteExisting_stateChanged(int);
        void on_chkPurge_stateChanged(int);
        void on_chkKeepBroken_stateChanged(int);
        void unpackStart(const app::Archive& arc);
        void unpackReady(const app::Archive& arc);
        void unpackProgress(const QString& target, int done);

    private:
        enum class MoveDirection {
            Top, Up, Down, Bottom
        };

        void moveTasks(MoveDirection dir);

    private:
        Ui::Unpack ui_;
    private:
        app::Unpacker& model_;
    private:
        std::size_t numUnpacks_;
    };

} // gui