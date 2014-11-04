// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
//
// http://www.ensisoft.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QFileDialog>
#  include <QtGui/QMessageBox>
#include <newsflash/warnpop.h>
#include <newsflash/keygen/keygen.h>
#include "dlgfeedback.h"
#include "../platform.h"

namespace gui
{

DlgFeedback::DlgFeedback(QWidget* parent, mode m) : QDialog(parent), uimode_(m)
{
    const auto& fingerprint = keygen::generate_fingerprint();
    const auto& platform = app::get_platform_name();    

    ui_.setupUi(this);
    ui_.editVersion->setText(NEWSFLASH_VERSION);
    ui_.editPlatform->setText(platform);
    ui_.progress->setVisible(false);
    ui_.progress->setRange(0, 0);
    ui_.progress->setValue(0);
    ui_.cmbFeelings->setVisible(false);

    switch (m)
    {
        case mode::bugreport:
            setWindowTitle("Report a Bug");
            setWindowIcon(QIcon(":/resource/16x16_ico_png/ico_bug.png"));
            ui_.grpMessage->setTitle("Description *");
            ui_.txtMessage->setPlainText(
                "Description of the problem:\r\n\r\n"
                "Steps to reproduce the problem:\r\n\r\n"
                "1.\r\n"
                "2.\r\n"
                "3.\r\n");
            break;

        case mode::feedback:
            setWindowTitle("Send feedback");
            setWindowIcon(QIcon(":/resource/16x16_ico_png/ico_user_feedback.png"));
            ui_.grpMessage->setTitle("Message *");
            ui_.cmbFeelings->setVisible(true);
            break;

        case mode::request_feature:
            setWindowTitle("Request a Feature");
            setWindowIcon(QIcon(":/resource/16x16_ico_png/ico_idea.png"));
            ui_.grpMessage->setTitle("Request *");
            break;

        case mode::request_license:
            setWindowTitle("Request Registration License");
            setWindowIcon(QIcon(":/resource/16x16_ico_png/ico_register.png"));
            ui_.grpMessage->setTitle("Request License");
            ui_.txtMessage->setPlainText(
                QString(
                    "Hello,\r\n\r\n"
                    "I've made a donation. Please send me the registration code.\r\n\r\n"
                    "ID: %1").arg(fingerprint));
            break;
    }   
}

DlgFeedback::~DlgFeedback()
{}

void DlgFeedback::on_btnSend_clicked()
{
    using feelings = app::feedback::feeling;

    app::feedback::message msg;
    msg.type    = uimode_;
    msg.feeling = (feelings)ui_.cmbFeelings->currentIndex();

    msg.name = ui_.editName->text();
    if (msg.name.isEmpty())
    {
        ui_.editName->setFocus();
        return;
    }

    msg.email = ui_.editEmail->text();
    if (msg.email.isEmpty())
    {
        ui_.editEmail->setFocus();
        return;
    }

    msg.platform = ui_.editPlatform->text();
    if (msg.platform.isEmpty())
    {
        ui_.editPlatform->setFocus();
        return;
    }

    msg.version = ui_.editVersion->text();
    if (msg.version.isEmpty())
    {
        ui_.editVersion->setFocus();
        return;
    }

    msg.text = ui_.txtMessage->toPlainText();
    if (msg.text.isEmpty())
    {
        ui_.txtMessage->setFocus();
        return;
    }

    if (ui_.grpAttachment->isChecked())
    {
        msg.attachment = ui_.editAttachment->text();
        if (msg.attachment.isEmpty())
        {
            ui_.editAttachment->setFocus();
            return;
        }
    }

    msg.country = ui_.editCountry->text();

    feedback_.on_complete = [&](app::feedback::response r) {
        using response = app::feedback::response;

        ui_.progress->setVisible(false);
        ui_.btnSend->setEnabled(true);
        switch (r)
        {
            case response::success:
                QMessageBox::information(this, windowTitle(),
                    "Thank you. Your message has been sent.");
                break;
            case response::dirty_rotten_spammer:
                QMessageBox::critical(this, windowTitle(),
                    "You're submitting data too fast. Please hold off for a moment!");
                break;

            case response::database_unavailable:
            case response::database_error:
            case response::email_unavailable:
                QMessageBox::critical(this, windowTitle(),
                    "A database error occurred. Your message was not recorded. Please try again later.");
                break;

            case response::network_error:
                QMessageBox::critical(this, windowTitle(),
                    "A network error occurred. Your message was not delivered. Please try again later.");
                break;
        }
    };
    feedback_.send(msg);

    ui_.progress->setVisible(true);
    ui_.btnSend->setEnabled(false);
}

void DlgFeedback::on_btnClose_clicked()
{
    close();
}

void DlgFeedback::on_btnBrowse_clicked()
{
    const auto& file = QFileDialog::getOpenFileName();
    if (file.isEmpty())
        return;

    ui_.editAttachment->setText(file);
}

void DlgFeedback::on_txtMessage_textChanged()
{
    // update character count
    const auto& text = ui_.txtMessage->toPlainText();

    ui_.lblLimit->setText(QString("%1/%2").arg(text.size()).arg(2048));
}

} // gui