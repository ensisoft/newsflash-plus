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
#  include "ui_searchsettings.h"
#include "newsflash/warnpop.h"
#include <vector>
#include "mainmodule.h"
#include "settings.h"
#include "app/newznab.h"

namespace gui
{
    class SearchSettings : public SettingsWidget
    {
        Q_OBJECT

    public:
        SearchSettings(std::vector<app::Newznab::Account> newznab);
       ~SearchSettings();

        virtual bool validate() const override;

    private slots:
        void on_btnImport_clicked();
        void on_btnAdd_clicked();
        void on_btnDel_clicked();
        void on_btnEdit_clicked();
        void on_listServers_currentRowChanged(int currentRow);

    private:
        Ui::SearchSettings ui_;
    private:
        friend class SearchModule;
        std::vector<app::Newznab::Account> newznab_;
    };


    class SearchModule : public QObject, public MainModule
    {
        Q_OBJECT

    public:
        SearchModule();
       ~SearchModule();

        virtual void saveState(app::Settings& settings) override;
        virtual void loadState(app::Settings& settings) override;

        virtual MainWidget* openSearch() override;

        virtual SettingsWidget* getSettings() override;
        virtual void applySettings(SettingsWidget* gui) override;
        virtual void freeSettings(SettingsWidget* gui) override;

        const app::Newznab::Account& getAccount(const QString& apiurl) const;
    Q_SIGNALS:
        void listUpdated(const QStringList&);

    private:
        std::vector<app::Newznab::Account> newznab_;
    };

} // gui
