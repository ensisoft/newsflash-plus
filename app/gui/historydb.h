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
#  include "ui_historydb.h"
#include <newsflash/warnpop.h>
#include "mainmodule.h"
#include "settings.h"

namespace app {
    class HistoryDb;
}

namespace gui
{
    class HistoryDbSettings : public SettingsWidget
    {
        Q_OBJECT

    public:
        HistoryDbSettings(app::HistoryDb* model);
       ~HistoryDbSettings();

    private slots:
        void on_btnClear_clicked();
    private:
        friend class HistoryDb;
    private:
        Ui::History m_ui;
    private:
        app::HistoryDb* m_model;
    };

    // Interface module for HistoryDb
    class HistoryDb : public QObject, public MainModule
    {
        Q_OBJECT

    public:
        HistoryDb(app::HistoryDb* model);
       ~HistoryDb();

        virtual SettingsWidget* getSettings() override;
        virtual void applySettings(SettingsWidget* gui) override;
        virtual void freeSettings(SettingsWidget* gui) override;
    private:

        app::HistoryDb* m_model;
    };
} // gui