// Copyright (c) 2010-2017 Sami Väisänen, Ensisoft
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
#  include "ui_omdb.h"
#include "newsflash/warnpop.h"

#include "settings.h"
#include "mainmodule.h"
#include "dlgmovie.h"

namespace app {
    class MovieDatabase;
}

namespace gui
{
    class OmdbSettings : public gui::SettingsWidget
    {
        Q_OBJECT

    public:
        OmdbSettings(const QString& apikey)
        {
            ui_.setupUi(this);
            ui_.apikey->setText(apikey);
        }
        QString apikey() const
        {
            return ui_.apikey->text();
        }
    private slots:
        void on_btnTest_clicked()
        {
            QString key = ui_.apikey->text();
            if (key.isEmpty())
            {
                ui_.apikey->setFocus();
                return;
            }

            DlgMovie dlg(this);
            dlg.testLookupMovie(key);
            dlg.exec();
        }

    private:
        Ui::Omdb ui_;
    private:

    };


    class Omdb : public MainModule
    {
    public:
        Omdb(app::MovieDatabase* omdb)
          : omdb_(omdb)
        {}

        virtual void loadState(app::Settings& s) override;
        virtual void saveState(app::Settings& s) override;
        virtual SettingsWidget* getSettings() override;
        virtual void applySettings(SettingsWidget* w) override;
        virtual void freeSettings(SettingsWidget* w) override;

    private:
        app::MovieDatabase* omdb_ = nullptr;

    };
} // namespace
