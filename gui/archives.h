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
#  include "ui_archives.h"
#include "newsflash/warnpop.h"
#include "mainwidget.h"
#include "app/unpacker.h"

namespace gui
{
    class Unpack;
    class Repair;

    class Archives : public MainWidget
    {
        Q_OBJECT

    public:
        Archives(Unpack& unpack, Repair& repair);
       ~Archives();

        virtual void addActions(QToolBar& bar) override;
        virtual void addActions(QMenu& menu) override;
        virtual void activate(QWidget*) override;
        virtual void deactivate() override;
        virtual void loadState(app::Settings& settings) override;
        virtual void saveState(app::Settings& settings) override;
        virtual void shutdown() override;
        virtual void refresh(bool isActive) override;

        virtual info getInformation() const override
        { return {"archives.html", true}; }

        virtual bool dropFile(const QString& file) override;

    private slots:
        void on_actionRepair_triggered();
        void on_actionUnpack_triggered();
        void on_tabWidget_currentChanged(int index);

    private:
        MainWidget* getCurrent();

    private:
        Ui::Archives ui_;

    private:
        Unpack& unpack_;
        Repair& repair_;
        MainWidget* current_;
    };

} // gui