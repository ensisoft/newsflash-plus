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
#  include <QObject>
#  include "ui_tools.h"
#include <newsflash/warnpop.h>
#include "mainmodule.h"
#include "settings.h"
#include "../tools.h"

namespace gui
{
    class ToolSettings : public SettingsWidget
    {
        Q_OBJECT

    public:
        ToolSettings();
       ~ToolSettings();

        virtual bool validate() const override;

        void setTools(std::vector<app::tools::tool> tools);

    private slots:
        void on_btnAdd_clicked();
        void on_btnDel_clicked();
        void on_btnEdit_clicked();
        void on_btnMoveUp_clicked();
        void on_btnMoveDown_clicked();
        void on_listTools_doubleClicked(QModelIndex);

    private:
        Ui::Tools ui_;
    private:
        friend class ToolModule;
        std::vector<app::tools::tool> tools_;
    };

    class ToolModule : public MainModule
    {
    public:
        ToolModule();
       ~ToolModule();

        virtual SettingsWidget* getSettings() override;
        virtual void applySettings(SettingsWidget* gui) override;
        virtual void freeSettings(SettingsWidget* gui) override;

        virtual info getInformation() const override
        { return {"toolmodule", "",}; }
    private:
    };

} // gui