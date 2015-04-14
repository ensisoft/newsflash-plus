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

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#include <newsflash/warnpop.h>
#include <limits>
#include "dlgfilter.h"

namespace gui
{

int myLog(int value)
{
    for (int i=0; i<200; ++i)
    {
        if (std::pow(1.1, i) >= value)
            return i;
    }
    Q_ASSERT(0);
    return -1; 
}

DlgFilter::DlgFilter(QWidget* parent, DlgFilter::Params& params) : QDialog(parent), params_(params)
{
    ui_.setupUi(this);
    ui_.grpMaxDays->setChecked(params.bMaxDays);
    ui_.grpMinDays->setChecked(params.bMinDays),
    ui_.grpMinSize->setChecked(params.bMinSize);
    ui_.grpMaxSize->setChecked(params.bMaxSize);

    ui_.spinMaxDays->setMaximum(12527);
    ui_.spinMinDays->setMaximum(12527);
    ui_.spinMinDays->setValue(params.minDays);
    ui_.spinMaxDays->setValue(params.maxDays);

    ui_.spinMinSize->setMaximum(4 * 1024);
    ui_.spinMaxSize->setMaximum(4 * 1024);
    ui_.sliderMaxSize->setMaximum(myLog(4 * 1024));
    ui_.sliderMinSize->setMaximum(myLog(4 * 1024));

    switch (params.sizeUnits)
    {
        case Unit::KB: 
            ui_.chkKB->setChecked(true);         
            ui_.spinMinSize->setValue(params_.minSize / 1024);
            ui_.spinMaxSize->setValue(params_.maxSize / 1024);            
            break;
        case Unit::MB: 
            ui_.chkMB->setChecked(true);         
            ui_.spinMinSize->setValue(params_.minSize / (1024 * 1024));
            ui_.spinMaxSize->setValue(params_.maxSize / (1024 * 1024));            
            break;
        case Unit::GB: 
            ui_.chkGB->setChecked(true);         
            ui_.spinMinSize->setValue(params_.minSize / (1024 * 1024 * 1024));
            ui_.spinMaxSize->setValue(params_.maxSize / (1024 * 1024 * 1024));            
            break;
        default: Q_ASSERT(!"unknown filtering size unit"); break; 
    }

    ui_.btnApply->setEnabled(false);


    ui_.chkGB->setVisible(false);
}

bool DlgFilter::isApplied() const 
{
    return !ui_.btnApply->isEnabled();
}

void DlgFilter::on_btnAccept_clicked()
{
    setParams();
    accept();
}

void DlgFilter::on_btnCancel_clicked()
{
    reject();
}
void DlgFilter::on_btnApply_clicked()
{
    setParams();

    applyFilter(params_.minDays, params_.maxDays, params_.minSize, params_.maxSize);

    ui_.btnApply->setEnabled(false);
}

void DlgFilter::on_sliderMinSize_valueChanged(int position)
{
    blockStupidSignals(true);

    int value = 0;
    if (position > 0)
        value = pow(1.1, position);

    // push "at most" forward so that it equals
    // the minimum
    if (value > ui_.spinMaxSize->value())
    {
        ui_.spinMaxSize->setValue(value);
        ui_.sliderMaxSize->setValue(position);
    }
    ui_.spinMinSize->setValue(value);

    ui_.btnApply->setEnabled(true);

    blockStupidSignals(false);
}    

void DlgFilter::on_sliderMaxSize_valueChanged(int position)
{
    blockStupidSignals(true);

    int value = 0;
    if (position > 0)
        value = pow(1.1, position);

    if (value < ui_.spinMinSize->value())
    {
        ui_.spinMinSize->setValue(value);
        ui_.sliderMinSize->setValue(position);
    }

    ui_.spinMaxSize->setValue(value);

    ui_.btnApply->setEnabled(true);    

    blockStupidSignals(false);
}

void DlgFilter::on_spinMinSize_valueChanged(int value)
{
    blockStupidSignals(true);

    ui_.sliderMinSize->setValue(myLog(value));

    if (value > ui_.spinMaxSize->value())
        ui_.spinMaxSize->setValue(value);

    ui_.btnApply->setEnabled(true);    

    blockStupidSignals(false);
}

void DlgFilter::on_spinMaxSize_valueChanged(int value)
{
    blockStupidSignals(true);

    ui_.sliderMaxSize->setValue(myLog(value));

    if (value < ui_.spinMinSize->value())
        ui_.spinMinSize->setValue(value);
    
    ui_.btnApply->setEnabled(true);    

    blockStupidSignals(false);
}

void DlgFilter::on_spinMaxDays_valueChanged(int value)
{
    blockStupidSignals(true);

    ui_.sliderMaxDays->setValue(myLog(value));

    if (value < ui_.spinMinDays->value())
        ui_.spinMinDays->setValue(value);

    ui_.btnApply->setEnabled(true);    

    blockStupidSignals(false);
}

void DlgFilter::on_spinMinDays_valueChanged(int value)
{
    blockStupidSignals(true);

    ui_.sliderMinDays->setValue(myLog(value));

    if (value > ui_.spinMaxDays->value())
        ui_.spinMaxDays->setValue(value);

    ui_.btnApply->setEnabled(true);    

    blockStupidSignals(false);
}

void DlgFilter::on_sliderMaxDays_valueChanged(int position)
{
    blockStupidSignals(true);

    int value = 0;
    if (position > 0)
        value = pow(1.1, position);

    if (value < ui_.spinMinDays->value())
    {
        ui_.spinMinDays->setValue(value);
        ui_.sliderMinDays->setValue(position);
    }
    ui_.spinMaxDays->setValue(value);

    ui_.btnApply->setEnabled(true);    

    blockStupidSignals(false);
}

void DlgFilter::on_sliderMinDays_valueChanged(int position)
{
    blockStupidSignals(true);

    int value = 0;
    if (position > 0) 
        value = pow(1.1, position);

    if (value > ui_.spinMaxDays->value())
    {
        ui_.spinMaxDays->setValue(value);
        ui_.sliderMaxDays->setValue(position);
    }
    ui_.spinMinDays->setValue(value);

    ui_.btnApply->setEnabled(true);

    blockStupidSignals(false);
}

void DlgFilter::on_chkKB_clicked()
{
    ui_.btnApply->setEnabled(true);
}

void DlgFilter::on_chkMB_clicked()
{
    ui_.btnApply->setEnabled(true);
}

void DlgFilter::on_chkGB_clicked()
{
    ui_.btnApply->setEnabled(true);
}

void DlgFilter::blockStupidSignals(bool block)
{
    ui_.sliderMaxDays->blockSignals(block);
    ui_.sliderMinDays->blockSignals(block);
    ui_.sliderMinSize->blockSignals(block);
    ui_.sliderMaxSize->blockSignals(block);
    ui_.spinMaxDays->blockSignals(block);
    ui_.spinMinDays->blockSignals(block);
    ui_.spinMaxSize->blockSignals(block);
    ui_.spinMinSize->blockSignals(block);
}

void DlgFilter::setParams()
{
    quint64 sizeMultiplier = 0;

    if (ui_.chkKB->isChecked())
    {
        sizeMultiplier = 1024;
        params_.sizeUnits = Unit::KB;
    }
    else if (ui_.chkMB->isChecked())
    {
        sizeMultiplier = 1024 * 1024;
        params_.sizeUnits = Unit::MB;
    }
    else if (ui_.chkGB->isChecked())
    {
        sizeMultiplier = 1024 * 1024 * 1024;
        params_.sizeUnits = Unit::GB;
    }

    params_.minSize = sizeMultiplier * ui_.spinMinSize->value();
    params_.maxSize = sizeMultiplier * ui_.spinMaxSize->value();
    params_.minDays = ui_.spinMinDays->value();
    params_.maxDays = ui_.spinMaxDays->value();    
}

} // gui