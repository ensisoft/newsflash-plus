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
#  include "ui_coresettings.h"
#include <newsflash/warnpop.h>
#include <memory>
#include "mainmodule.h"
#include "settings.h"

namespace app {
    class Telephone;
} // app

namespace gui
{
    // have to be namespace scope class because MOC doesnt support
    // shitty signals and slots for nested classes... 
    class CoreSettings  : public SettingsWidget
    {
        Q_OBJECT

    public:
        CoreSettings();
       ~CoreSettings();

        virtual bool validate() const override;
    private slots:
        void on_btnBrowseLog_clicked();
        void on_btnBrowseDownloads_clicked();
        void on_sliderThrottle_valueChanged(int val);

    private:
        Ui::CoreSettings ui_;
    private:
        friend class CoreModule;
    };

    // glue code for general application settings and engine + feedback system
    // translates settings data to UI state and vice versa.
    class CoreModule : public QObject, public MainModule
    {
        Q_OBJECT

    public:
        CoreModule();
       ~CoreModule();

        virtual void loadState(app::Settings& s) override;       
        virtual void saveState(app::Settings& s) override;
        virtual SettingsWidget* getSettings() override;
        virtual void applySettings(SettingsWidget* gui) override;
        virtual void freeSettings(SettingsWidget* gui) override;

        virtual info getInformation() const override
        { return {"coremodule", ""}; }

    private slots:
        void calledHome(bool new_version_available, QString latest);

    private:
        std::unique_ptr<app::Telephone> et_;
    private:
        bool check_for_updates_;
    };
} // gui