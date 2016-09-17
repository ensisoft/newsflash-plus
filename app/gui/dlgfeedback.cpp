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

#include "config.h"

#include "warnpush.h"
#  include <QtGui/QFileDialog>
#  include <QtGui/QMessageBox>
#include "warnpop.h"

#include "tools/keygen/keygen.h"

#include "dlgfeedback.h"
#include "../platform.h"

namespace gui
{

DlgFeedback::DlgFeedback(QWidget* parent, mode m) : QDialog(parent), uimode_(m)
{
    const auto& fingerprint = keygen::generate_fingerprint();
    const auto& platform = app::getPlatformName();

    ui_.setupUi(this);
    ui_.editVersion->setText(feedback_.version());
    ui_.editPlatform->setText(feedback_.platform());
    ui_.progress->setVisible(false);
    ui_.progress->setRange(0, 0);
    ui_.progress->setValue(0);
    ui_.cmbFeelings->setVisible(false);

    switch (m)
    {
        case mode::BugReport:
            setWindowTitle("Report a Bug");
            setWindowIcon(QIcon("icons:ico_app_report_bug.png"));
            ui_.grpMessage->setTitle("Description *");
            ui_.txtMessage->setPlainText(
                "Description of the problem:\r\n\r\n"
                "Steps to reproduce the problem:\r\n\r\n"
                "1.\r\n"
                "2.\r\n"
                "3.\r\n");
            break;

        case mode::FeedBack:
            setWindowTitle("Send feedback");
            setWindowIcon(QIcon("icons:ico_app_send_feedback.png"));
            ui_.grpMessage->setTitle("Message *");
            ui_.cmbFeelings->setVisible(true);
            break;

        case mode::FeatureRequest:
            setWindowTitle("Request a Feature");
            setWindowIcon(QIcon("icons:ico_app_request_feature.png"));
            ui_.grpMessage->setTitle("Request *");
            break;

        case mode::LicenseRequest:
            setWindowTitle("Request Registration License");
            setWindowIcon(QIcon("icons:ico_app_request_license.png"));
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
    using Feelings = app::Feedback::Feeling;
    using Response = app::Feedback::Response;

#define MUST_HAVE(x) \
    if (x->text().isEmpty()) {\
        x->setFocus(); \
        return; \
    }

    MUST_HAVE(ui_.editName);
    MUST_HAVE(ui_.editEmail);
    MUST_HAVE(ui_.editPlatform);
    MUST_HAVE(ui_.editVersion);

    feedback_.setType(uimode_);
    feedback_.setFeeling((Feelings)ui_.cmbFeelings->currentIndex());
    feedback_.setName(ui_.editName->text());
    feedback_.setEmail(ui_.editEmail->text());
    feedback_.setPlatform(ui_.editPlatform->text());
    feedback_.setVersion(ui_.editVersion->text());
    feedback_.setMessage(ui_.txtMessage->toPlainText());

    if (ui_.grpAttachment->isChecked())
    {
        MUST_HAVE(ui_.editAttachment);
        feedback_.setAttachment(ui_.editAttachment->text());
    }

    feedback_.setCountry(ui_.editCountry->text());

    feedback_.onComplete = [&](app::Feedback::Response r) {
        ui_.progress->setVisible(false);
        ui_.btnSend->setEnabled(true);
        switch (r)
        {
            case Response::Success:
                QMessageBox::information(this, windowTitle(),
                    "Thank you. Your message has been sent.");
                break;
            case Response::DirtyRottenSpammer:
                QMessageBox::critical(this, windowTitle(),
                    "You're submitting data too fast. Please hold off for a moment!");
                break;

            case Response::DatabaseUnavailable:
            case Response::DatabaseError:
            case Response::EmailUnavailable:
                QMessageBox::critical(this, windowTitle(),
                    "A database error occurred. Your message was not recorded. Please try again later.");
                break;

            case Response::NetworkError:
                QMessageBox::critical(this, windowTitle(),
                    "A network error occurred. Your message was not delivered. Please try again later.");
                break;
        }
    };
    feedback_.send();

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