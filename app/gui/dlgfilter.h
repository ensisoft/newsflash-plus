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
        enum class Unit {
            KB, MB, GB
        };

        struct Params {
            quint32 minDays;
            quint32 maxDays;
            quint64 minSize;
            quint64 maxSize;
            bool bMinDays;
            bool bMaxDays;
            bool bMinSize;
            bool bMaxSize;
            Unit sizeUnits;
        };

        DlgFilter(QWidget* parent, Params& params);

        std::function<void(quint32 minDays, quint32 maxDays,
            quint64 minSize, quint64 maxSize)> applyFilter;

        bool isApplied() const;
    private slots:
        void on_btnAccept_clicked();
        void on_btnCancel_clicked();
        void on_btnApply_clicked();
        void on_sliderMinSize_valueChanged(int position);
        void on_sliderMaxSize_valueChanged(int position);
        void on_spinMinSize_valueChanged(int value);
        void on_spinMaxSize_valueChanged(int value);
        void on_spinMaxDays_valueChanged(int value);
        void on_spinMinDays_valueChanged(int value);
        void on_sliderMaxDays_valueChanged(int position);
        void on_sliderMinDays_valueChanged(int position);
        void on_chkKB_clicked();
        void on_chkMB_clicked();
        void on_chkGB_clicked();

    private:
        void blockStupidSignals(bool block);           
        void setParams();

    private:
        Ui::Filter ui_;
    private:
        Params& params_;
    };
} // gui