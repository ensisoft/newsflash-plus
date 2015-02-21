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

#include <newsflash/warnpush.h>
#  include <QObject>
#  include "ui_commands.h"
#include <newsflash/warnpop.h>
#include "mainmodule.h"
#include "settings.h"
#include "../commands.h"

namespace gui
{
    // this is the settings UI for the Commands module.
    // however it has to be in the namespace scope because of MOC.
    class CmdSettings : public SettingsWidget
    {
        Q_OBJECT

    public:
        CmdSettings(app::Commands::CmdList cmds);
       ~CmdSettings();

        virtual bool validate() const override
        { return true; }

    private slots:
        void on_btnAdd_clicked();
        void on_btnDel_clicked();
        void on_btnEdit_clicked();
        void on_btnMoveUp_clicked();
        void on_btnMoveDown_clicked();
        void on_actionAdd_triggered();
        void on_actionDel_triggered();
        void on_actionEdit_triggered();
        void on_tableCmds_customContextMenuRequested(QPoint);
        void tableCmds_selectionChanged();
    private:
        Ui::Commands ui_;
    private:
        class Model;
        std::unique_ptr<Model> model_;
    };

    class Commands : public MainModule
    {
    public:
        virtual SettingsWidget* getSettings() override
        {
            auto cmds = commands_.getCommandsCopy();
            return new CmdSettings(cmds);
        }
        virtual void applySettings(SettingsWidget* gui) override
        {

        }

        virtual void loadState(app::Settings& settings) override
        {
            commands_.loadState(settings);
        }

        virtual bool saveState(app::Settings& settings) override
        { 
            commands_.saveState(settings);
            return true;
        }
        virtual void firstLaunch() 
        {
            // todo:
        }

        virtual void freeSettings(SettingsWidget* gui) override
        { delete gui; }
    private:

        app::Commands commands_;

    };
} // gui
