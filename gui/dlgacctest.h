// Copyright (c) 2010-2016 Sami Väisänen, Ensisoft
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
#  include <QtGui/QPixmap>
#  include <QString>
#  include "ui_dlgacctest.h"
#include "newsflash/warnpop.h"

#include "app/engine.h"

namespace gui
{
    class DlgAccTest : public QDialog
    {
        Q_OBJECT

    public:
        DlgAccTest(QWidget* parent) : QDialog(parent)
        {
            m_ui.setupUi(this);

            QObject::connect(app::g_engine, SIGNAL(testAccountComplete(bool)),
                this, SLOT(testAccountComplete(bool)));
            QObject::connect(app::g_engine, SIGNAL(testAccountLogMsg(const QString&)),
                this, SLOT(testAccountLogMsg(const QString&)));
        }

        void start()
        {
            m_ui.progressBar->setMaximum(0);
            m_ui.progressBar->setMinimum(0);
            m_ui.progressBar->setValue(0);
            m_ui.progressBar->setVisible(true);
            m_ui.textEdit->clear();
            m_ui.lblMessage->setVisible(false);
            m_ui.lblIcon->setVisible(false);
        }

    private slots:
        void on_btnClose_clicked()
        {
            close();
        }

        void testAccountComplete(bool success)
        {
            m_ui.progressBar->setVisible(false);
            m_ui.lblMessage->setVisible(true);
            m_ui.lblIcon->setVisible(true);
            if (success)
            {
                m_ui.lblMessage->setText(tr("Good to go!"));
                m_ui.lblIcon->setPixmap(QPixmap("pixmaps:success.png"));
            }
            else
            {
                m_ui.lblMessage->setText(tr("Sorry, something went wrong.\r\n"
                    "See the log for details."));
                m_ui.lblIcon->setPixmap(QPixmap("pixmaps:failure.png"));
            }
        }
        void testAccountLogMsg(const QString& msg)
        {
            QString m = msg;
            if (m.isEmpty())
                return;
            if (m[m.size()-1] == '\n')
                m.resize(m.size()-1);
            if (m[m.size()-1] == '\r')
                m.resize(m.size()-1);
            m_ui.textEdit->append(m);
        }

    private:

        Ui::DlgAccTest m_ui;

    };
} // gui