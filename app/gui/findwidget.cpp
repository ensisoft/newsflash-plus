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
#  include <QtGui/QMessageBox>
#include <newsflash/warnpop.h>
#include "findwidget.h"
#include "finder.h"

namespace gui
{

FindWidget::FindWidget(QWidget* parent, Finder& finder) : QWidget(parent), finder_(finder)
{
    ui_.setupUi(this);
}

FindWidget::~FindWidget()
{}

void FindWidget::find()
{
    ui_.editFind->setFocus();
    ui_.editFind->selectAll();
}

void FindWidget::findNext()
{
    doFind(true);
}

void FindWidget::findPrev()
{
    doFind(false);
}

void FindWidget::on_btnPrev_clicked()
{
    doFind(false);
}

void FindWidget::on_btnNext_clicked()
{
    doFind(true);
}

void FindWidget::on_btnQuit_clicked()
{
    close();
}

void FindWidget::on_editFind_returnPressed()
{
    doFind(true);
}

void FindWidget::on_editFind_textEdited()
{
    // set to empty, isEmpty will be true
    regexp_.setPattern("");
}

void FindWidget::doFind(bool forward)
{
    const auto regexp = ui_.chkRegExp->isChecked();
    const auto invert = ui_.chkInvert->isChecked();
    const auto matchcase = ui_.chkMatchCase->isChecked();
    const auto str = ui_.editFind->text();

    ui_.label->setVisible(false);

    if (regexp)
    {
        if (regexp_.isEmpty())
        {
            regexp_.setPattern(str);
            regexp_.setCaseSensitivity(matchcase ? Qt::CaseSensitive :
                Qt::CaseInsensitive);
        }
        if (!regexp_.isValid())
        {
            const auto err = regexp_.errorString();
            QMessageBox::critical(this, 
                tr("Incorrect Regular Expression"), err);            
            return;
        }
        auto numItems = finder_.numItems();
        auto curItem  = finder_.curItem();
        for (std::size_t i=0; i<numItems; ++i)
        {
            if (forward)
                curItem = (curItem + 1) % numItems;
            else if (curItem == 0)
                curItem = numItems - 1;
            else curItem--;

            if (finder_.isMatch(regexp_, curItem))
            {
                finder_.setFound(curItem);
                return;
            }
        }
    }
    else
    {
        auto pattern = str;
        if (!matchcase)
            pattern = str.toUpper();

        auto numItems = finder_.numItems();
        auto curItem  = finder_.curItem();
        for (std::size_t i=0; i<numItems; ++i)
        {
            if (forward)
                curItem = (curItem + 1) % numItems;
            else if (curItem == 0)
                curItem = numItems - 1;
            else curItem--;

            if (finder_.isMatch(pattern, curItem, matchcase))
            {
                finder_.setFound(curItem);
                return;
            }
        }
    }

    ui_.label->setText("No matches");
    ui_.label->setVisible(true);
}

} // gui
