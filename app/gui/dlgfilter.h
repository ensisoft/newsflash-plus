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

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QDialog>
#  include "ui_filter.h"
#include <newsflash/warnpop.h>
#include <functional>

namespace gui
{
    class DlgFilter : public QDialog
    {
        Q_OBJECT

    public:
        DlgFilter(QWidget* parent) : QDialog(parent)
        {
            ui_.setupUi(this);
            ui_.chkKB->setChecked(true);
        }

        std::function<void(int minAge, int maxAge,
            int minSize, int maxSize)> applyFilter;

    private slots:
        void on_btnAccept_clicked()
        {
            accept();
        }
        void on_btnCancel_clicked()
        {
            reject();
        }
        void on_btnApply_clicked()
        {
            int sizeMultiplier = 0;
            int minSize = -1;
            int maxSize = -1;
            if (ui_.chkKB->isChecked())
                sizeMultiplier = 1024;
            else if (ui_.chkMB->isChecked())
                sizeMultiplier = 1024 * 1024;
            else if (ui_.chkGB->isChecked())
                sizeMultiplier = 1024 * 1024 * 1024;

            if (ui_.grpAtLeast->isChecked())
                minSize = sizeMultiplier * ui_.spinAtLeast->value();
            if (ui_.grpAtMost->isChecked())
                maxSize = sizeMultiplier * ui_.spinAtMost->value();

            int minAge = -1;
            int maxAge = -1;


            //applyFilter(minAge, maxAge, minSize, maxSize);
        }

    private:
        Ui::Filter ui_;
    };
} // gui