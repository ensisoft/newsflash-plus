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
#  include <QObject>
#  include "ui_nzbcore.h"
#include "newsflash/warnpop.h"
#include "mainwidget.h"
#include "mainmodule.h"
#include "settings.h"
#include "app/nzbcore.h"

namespace gui
{
    class nzbfile;

    class NZBSettings : public SettingsWidget
    {
        Q_OBJECT
    public:
        NZBSettings();
       ~NZBSettings();

        virtual bool validate() const override;

    private slots:
        void on_btnAddWatchFolder_clicked();
        void on_btnDelWatchFolder_clicked();
    private:
        Ui::NZBCore ui_;
    private:
        friend class NZBCore;
    };

    // nzbcore UI functionality
    class NZBCore : public MainModule
    {
    public:
        NZBCore();
       ~NZBCore();

        virtual void loadState(app::Settings& s) override;
        virtual void saveState(app::Settings& s) override;
        virtual SettingsWidget* getSettings() override;
        virtual void applySettings(SettingsWidget* gui) override;
        virtual void freeSettings(SettingsWidget* s) override;
        virtual bool dropFile(const QString& file) override;
        virtual info getInformation() const override
        { return {"nzbcore", "nzb.html"}; }

    private:
        enum class DragDropAction {
            AskForAction,
            ShowContents,
            DownloadContents
        };
        DragDropAction m_action;
    private:
        app::NZBCore m_module;

    };
} // gui
