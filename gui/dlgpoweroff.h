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
#  include <QDialog>
#  include <QMessageBox>
#  include "ui_dlgpoweroff.h"
#include "newsflash/warnpop.h"

#include "app/poweroff.h"
#include "app/smtpclient.h"
#include "app/reporter.h"

namespace gui
{
    class DlgPoweroff : public QDialog
    {
        Q_OBJECT

    public:
        DlgPoweroff(QWidget* parent, bool sendReport) : QDialog(parent)
        {
            mUI.setupUi(this);
            mUI.btnDownloads->setChecked(app::g_poweroff->waitDownloads());
            mUI.btnRepairs->setChecked(app::g_poweroff->waitRepairs());
            mUI.btnUnpacks->setChecked(app::g_poweroff->waitUnpacks());
            if (sendReport)
            {
                mUI.chkSendEmail->setChecked(true);
            }
            else
            {
                mUI.chkSendEmail->setChecked(app::g_reporter->isJournaling());
            }
        }

        bool isPoweroffEnabled() const
        {
            return app::g_poweroff->isPoweroffEnabled();
        }

        bool configureSmtp() const
        { return mConfigureSmtp; }

        bool sendReport() const
        { return mUI.chkSendEmail->isChecked(); }

    private slots:
        void on_btnClose_clicked()
        {

            bool sendReport = mUI.chkSendEmail->isChecked();
            if (sendReport)
            {
                if (!app::g_smtp->configured())
                {
                    QMessageBox msg(this);
                    msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                    msg.setIcon(QMessageBox::Question);
                    msg.setText(tr("In order to send email you need to configure your email server.\r\n"
                        "Would you like to do this now?"));
                    const auto ret = msg.exec();
                    if (ret == QMessageBox::Yes)
                    {
                        mConfigureSmtp = true;
                        close();
                        return;
                    }
                    else
                    {
                        sendReport = false;
                    }
                }
            }

            app::g_poweroff->waitDownloads(mUI.btnDownloads->isChecked());
            app::g_poweroff->waitRepairs(mUI.btnRepairs->isChecked());
            app::g_poweroff->waitUnpacks(mUI.btnUnpacks->isChecked());
            if (sendReport)
            {
                app::g_reporter->enableJournal();
            }
            else
            {
                app::g_reporter->cancelJournal();
            }
            close();
        }

    private:
        Ui::Poweroff mUI;

    private:
        bool mConfigureSmtp = false;
    };
} // gui