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
#  include <QtGui/QDialog>
#  include <QtGlobal>
#  include "ui_dlglowdisk.h"
#include "newsflash/warnpop.h"
#include "app/types.h"
#include "app/format.h"

namespace gui
{
    class DlgLowDisk : public QDialog
    {
        Q_OBJECT

    public:
        DlgLowDisk(QWidget* parent, const QString& description,
            const QString& diskPartition,
            quint64 queuedBytes,
            quint64 requiredSize,
            quint64 availableSize) : QDialog(parent)
        {
            m_ui.setupUi(this);

            if (queuedBytes)
            {
                QString str = app::toString("The disk partition %1 "
                    "has only %2 available and %3 is already queued.\n"
                    "However the download\n%4\n"
                    "requires approx. %5 of space unpacked.",
                    diskPartition,
                    app::size { availableSize },
                    app::size { queuedBytes },
                    description,
                    app::size {requiredSize });
                m_ui.lblMessage->setText(str);
            }
            else
            {
                QString str = app::toString("The disk partition %1 "
                    "has only %2 available.\n"
                    "However the download\n%3\n"
                    "requires approx. %5 of space unpacked.",
                    diskPartition,
                    app::size { availableSize },
                    description,
                    app::size { requiredSize });
                m_ui.lblMessage->setText(str);
            }
        }

        bool checkLowDisk() const
        {
            return m_ui.chkLowDisk->isChecked();
        }
    private slots:
        void on_btnAccept_clicked()
        {
            accept();
        }
        void on_btnCancel_clicked()
        {
            reject();
        }
    private:
        Ui::DlgLowDisk m_ui;
    };
} // gui