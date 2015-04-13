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

DlgFilter::DlgFilter(QWidget* parent, DlgFilter::Params& params) : QDialog(parent), params_(params)
{
    ui_.setupUi(this);
    ui_.grpMaxDays->setChecked(params.bMaxDays);
    ui_.grpMinDays->setChecked(params.bMinDays),
    ui_.grpMinSize->setChecked(params.bMinSize);
    ui_.grpMaxSize->setChecked(params.bMaxSize);
    ui_.sliderMinSize->setValue(params.minSize);
    ui_.sliderMaxSize->setValue(params.maxSize);
    ui_.sliderMinDays->setValue(params.minDays);
    ui_.sliderMaxDays->setValue(params.maxDays);

    ui_.spinMaxDays->setMaximum(12527);
    ui_.spinMinDays->setMaximum(12527);
    ui_.spinMinSize->setMaximum(189905276);
    ui_.spinMaxSize->setMaximum(189905276);

    ui_.spinMinSize->setValue(params.minSize);
    ui_.spinMaxSize->setValue(params.maxSize);
    ui_.spinMinDays->setValue(params.minDays);
    ui_.spinMaxDays->setValue(params.maxDays);
    switch (params.sizeUnits)
    {
        case Unit::KB: ui_.chkKB->setChecked(true); break;
        case Unit::MB: ui_.chkMB->setChecked(true); break;
        case Unit::GB: ui_.chkGB->setChecked(true); break;
    }
}

void DlgFilter::on_btnAccept_clicked()
{
    params_.minDays = ui_.spinMinDays->value();    
    params_.maxDays = ui_.spinMaxDays->value();
    params_.minSize = ui_.spinMinSize->value();
    params_.maxSize = ui_.spinMaxSize->value();
    if (ui_.chkKB->isChecked())
        params_.sizeUnits = Unit::KB;
    else if (ui_.chkMB->isChecked())
        params_.sizeUnits = Unit::MB;
    else if (ui_.chkGB->isChecked())
        params_.sizeUnits = Unit::GB;

    accept();
}

void DlgFilter::on_btnCancel_clicked()
{
    reject();
}
void DlgFilter::on_btnApply_clicked()
{
    quint32 sizeMultiplier = 0;

    if (ui_.chkKB->isChecked())
        sizeMultiplier = 1024;
    else if (ui_.chkMB->isChecked())
        sizeMultiplier = 1024 * 1024;
    else if (ui_.chkGB->isChecked())
        sizeMultiplier = 1024 * 1024 * 1024;

    const quint32 minSize = sizeMultiplier * ui_.spinMinSize->value();
    const quint32 maxSize = sizeMultiplier * ui_.spinMaxSize->value();
    const quint32 minAge = ui_.spinMinDays->value();
    const quint32 maxAge = ui_.spinMaxDays->value();
    applyFilter(minAge, maxAge, minSize, maxSize);
}

void DlgFilter::on_sliderMinSize_valueChanged(int position)
{
    int value = pow(1.1, position);

            // push "at most" forward so that it equals
            // the minimum
    if (value > ui_.spinMaxSize->value())
    {
        ui_.spinMaxSize->setValue(value);
        ui_.sliderMaxSize->setValue(position);
    }
    ui_.spinMinSize->setValue(value);
}    

void DlgFilter::on_sliderMaxSize_valueChanged(int position)
{
    int value = pow(1.1, position);

    if (value < ui_.spinMinSize->value())
    {
        ui_.spinMinSize->setValue(value);
        ui_.sliderMinSize->setValue(position);
    }

    ui_.spinMaxSize->setValue(value);
}

void DlgFilter::on_spinMinSize_valueChanged(int value)
{
            // uh, log 1.1 (value) 
    for (int x=0; x<201; ++x)
    {
        if ((int)pow(1.1, x) == value)
            ui_.sliderMinSize->setValue(x);
    }

    if (value > ui_.spinMaxSize->value())
        ui_.spinMaxSize->setValue(value);
}

void DlgFilter::on_spinMaxSize_valueChanged(int value)
{
    for (int x=0; x<201; ++x)
    {
        if ((int)pow(1.1, x) == value)
            ui_.sliderMaxSize->setValue(x);
    }
    if (value < ui_.spinMinSize->value())
        ui_.spinMinSize->setValue(value);
    
}

void DlgFilter::on_spinMaxDays_valueChanged(int value)
{
    for (int x=0; x<201; ++x)
    {
        if ((int)pow(1.1, x) == value)
            ui_.sliderMaxDays->setValue(x);
    }
    if (value < ui_.spinMinDays->value())
        ui_.spinMinDays->setValue(value);
}

void DlgFilter::on_spinMinDays_valueChanged(int value)
{
    for (int x=0; x<201; ++x)
    {
        if ((int)pow(1.1, x) == value)
            ui_.sliderMinDays->setValue(x);
    }
    if (value > ui_.spinMaxDays->value())
        ui_.spinMaxDays->setValue(value);
}

void DlgFilter::on_sliderMaxDays_valueChanged(int position)
{
    int value = pow(1.1, position);

    if (value < ui_.spinMinDays->value())
    {
        ui_.spinMinDays->setValue(value);
        ui_.sliderMinDays->setValue(position);
    }
    ui_.spinMaxDays->setValue(value);
}

void DlgFilter::on_sliderMinDays_valueChanged(int position)
{
    int value = pow(1.1, position);

    if (value > ui_.spinMaxDays->value())
    {
        ui_.spinMaxDays->setValue(value);
        ui_.sliderMaxDays->setValue(position);
    }
    ui_.spinMinDays->setValue(value);
}


} // gui