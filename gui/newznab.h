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
#  include <QStringList>
#  include "ui_newznab.h"
#include "newsflash/warnpop.h"

#include <vector>
#include <memory>

#include "mainmodule.h"
#include "settings.h"
#include "app/newznab.h"
#include "app/types.h"
#include "app/media.h"
#include "engine/bitflag.h"

namespace app {
    class Indexer;
    class RSSFeed;
} // app

namespace gui
{
    class NewznabSettings : public SettingsWidget
    {
        Q_OBJECT

    public:
        NewznabSettings(std::vector<app::newznab::Account> accounts);
       ~NewznabSettings();

        virtual bool validate() const override;

    private slots:
        void on_btnImport_clicked();
        void on_btnAdd_clicked();
        void on_btnDel_clicked();
        void on_btnEdit_clicked();
        void on_listServers_currentRowChanged(int currentRow);

    private:
        Ui::NewznabSettings mUi;
    private:
        friend class Newznab;
        std::vector<app::newznab::Account> mAccounts;
    };


    // manages the Newznab accounts.
    class Newznab : public QObject, public MainModule
    {
        Q_OBJECT

    public:
        Newznab();
       ~Newznab();

        // MainModule implementation
        virtual void saveState(app::Settings& settings) override;
        virtual void loadState(app::Settings& settings) override;
        virtual SettingsWidget* getSettings() override;
        virtual void applySettings(SettingsWidget* gui) override;
        virtual void freeSettings(SettingsWidget* gui) override;

        QStringList listAccounts() const;

        std::unique_ptr<app::Indexer> makeSearchEngine(const QString& hostName);
        std::unique_ptr<app::RSSFeed> makeRSSFeedEngine(const QString& hostName);

    Q_SIGNALS:
        void listUpdated(const QStringList&);

    private:
        const app::newznab::Account& findAccount(const QString& hostName) const;

    private:
        std::vector<app::newznab::Account> mAccounts;
        app::MediaTypeFlag mEnabledStreams;
    };

} // gui
