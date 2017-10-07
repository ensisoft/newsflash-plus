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
#  include <QObject>
#  include "ui_smtpclient.h"
#include "newsflash/warnpop.h"

#include "app/smtpclient.h"
#include "mainmodule.h"
#include "settings.h"

namespace gui
{
    class SmtpSettings : public SettingsWidget
    {
        Q_OBJECT

    public:
        SmtpSettings();

        virtual bool validate() const override;

    private slots:
        void on_btnTest_clicked();

    private:
        Ui::Smtp mUI;
    private:
        friend class SmtpClient;
    };


    class SmtpClient : public MainModule
    {
    public:
        SmtpClient(app::SmtpClient& smtp);

        virtual void loadState(app::Settings& settings) override;
        virtual void saveState(app::Settings& settings) override;
        virtual SettingsWidget* getSettings() override;
        virtual void applySettings(SettingsWidget* gui) override;
        virtual void freeSettings(SettingsWidget* gui) override;
        virtual info getInformation() const override
        { return {"smtp", "smtp.html"}; }

    private:
        app::SmtpClient& mSmtp;
    };
} // namespace
