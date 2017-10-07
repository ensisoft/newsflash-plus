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

#define LOGTAG "smtp"

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QtGui/QMessageBox>
#include "newsflash/warnpop.h"

#include "smtpclient.h"
#include "common.h"
#include "app/debug.h"
#include "app/utility.h"
#include "app/format.h"
#include "app/settings.h"

namespace gui
{

SmtpSettings::SmtpSettings()
{
    mUI.setupUi(this);
    mUI.testIndicator->setVisible(false);
}

bool SmtpSettings::validate() const
{
    if (!gui::validate(mUI.edtHost))
        return false;
    if (!gui::validate(mUI.edtPort, EditValidationType::AcceptNumber))
        return false;
    if (!gui::validate(mUI.edtUsername))
        return false;
    if (!gui::validate(mUI.edtPassword))
        return false;
    if (!gui::validate(mUI.edtRecipient))
        return false;
    return true;
}

void SmtpSettings::on_btnTest_clicked()
{
    if (!validate())
        return;

    app::SmtpClient smtp;
    smtp.setEnabled(true);
    smtp.setPort(mUI.edtPort->text().toInt());
    smtp.setHost(mUI.edtHost->text());
    smtp.setSSL(mUI.chkSSL->isChecked());
    smtp.setUsername(mUI.edtUsername->text());
    smtp.setPassword(mUI.edtPassword->text());
    smtp.setRecipient(mUI.edtRecipient->text());

    mUI.testIndicator->setVisible(true);
    mUI.btnTest->setEnabled(false);

    const auto ret = smtp.sendEmailNow(tr("Test email from %1").arg(NEWSFLASH_TITLE),
        tr("Test email from %1. If you received this your email settings are working!").arg(NEWSFLASH_TITLE));

    mUI.testIndicator->setVisible(false);
    mUI.btnTest->setEnabled(true);

    if (ret == app::SmtpClient::Error::None)
    {
        QMessageBox msg(this);
        msg.setStandardButtons(QMessageBox::Ok);
        msg.setText(tr("Email was sent."));
        msg.setIcon(QMessageBox::Information);
        msg.exec();
    }
    else
    {
        QMessageBox msg(this);
        msg.setStandardButtons(QMessageBox::Ok);
        msg.setText(tr("There was a problem sending the email."));
        msg.setIcon(QMessageBox::Critical);
        msg.exec();
    }

}


SmtpClient::SmtpClient(app::SmtpClient& smtp) : mSmtp(smtp)
{}

void SmtpClient::loadState(app::Settings& settings)
{
    mSmtp.loadState(settings);
}

void SmtpClient::saveState(app::Settings& settings)
{
    mSmtp.saveState(settings);
}

SettingsWidget* SmtpClient::getSettings()
{
    SmtpSettings* settings = new SmtpSettings();

    auto& ui = settings->mUI;
    ui.grpEmail->setChecked(mSmtp.enabled());
    ui.edtPort->setText(QString::number(mSmtp.port()));
    ui.edtHost->setText(mSmtp.host());
    ui.chkSSL->setChecked(mSmtp.ssl());
    ui.edtUsername->setText(mSmtp.username());
    ui.edtPassword->setText(mSmtp.password());
    ui.edtRecipient->setText(mSmtp.recipient());
    return settings;
}

void SmtpClient::applySettings(SettingsWidget* gui)
{
    auto* settings = dynamic_cast<SmtpSettings*>(gui);
    auto& ui = settings->mUI;
    mSmtp.setEnabled(ui.grpEmail->isChecked());
    mSmtp.setPort(ui.edtPort->text().toInt());
    mSmtp.setHost(ui.edtHost->text());
    mSmtp.setSSL(ui.chkSSL->isChecked());
    mSmtp.setUsername(ui.edtUsername->text());
    mSmtp.setPassword(ui.edtPassword->text());
    mSmtp.setRecipient(ui.edtRecipient->text());
}

void SmtpClient::freeSettings(SettingsWidget* gui)
{
    delete gui;
}

} // namespace
