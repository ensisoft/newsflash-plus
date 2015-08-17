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

#define LOGTAG "history"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QMessageBox>
#include <newsflash/warnpop.h>
#include "historydb.h"
#include "../eventlog.h"
#include "../debug.h"
#include "../historydb.h"
#include "../settings.h"

namespace gui
{

HistoryDbSettings::HistoryDbSettings(app::HistoryDb* model) : m_model(model)
{
    m_ui.setupUi(this);
    m_ui.tableView->setModel(m_model);
    m_ui.btnClear->setEnabled(!m_model->isEmpty());
    m_ui.chkCheckDuplicates->setChecked(m_model->checkDuplicates());
    m_ui.chkExact->setChecked(m_model->exactMatching());
}

HistoryDbSettings::~HistoryDbSettings()
{}

void HistoryDbSettings::accept()
{
    bool checkDuplicates = m_ui.chkCheckDuplicates->isChecked();
    bool exactMatching = m_ui.chkExact->isChecked();

    m_model->checkDuplicates(checkDuplicates);
    m_model->exactMatching(exactMatching);
}

void HistoryDbSettings::on_btnClear_clicked()
{
    // todo: this action isn't reversible, 
    // i.e. if the settings dialog is canceled
    // the data is still already lost.
    // perhaps this should follow the same semantics?
    // then we don't need to ask here, but the user can just
    // cancel the settings and the data isnt lost.

    QMessageBox::StandardButton answer = QMessageBox::question(this, "Clear History",
        "Are you sure you want to clear the history?",
        QMessageBox::Yes | QMessageBox::No);
    if (answer == QMessageBox::No)
        return;

    m_model->clearHistory(true);
    m_ui.btnClear->setEnabled(false);
}

HistoryDb::HistoryDb(app::HistoryDb* model) : m_model(model)
{
    DEBUG("History UI created");
}

HistoryDb::~HistoryDb()
{
    DEBUG("History UI deleted");
}

void HistoryDb::loadState(app::Settings& settings)
{

}

void HistoryDb::saveState(app::Settings& settings)
{
    
}

SettingsWidget* HistoryDb::getSettings()
{
    auto* ptr = new HistoryDbSettings(m_model);

    return ptr;
}

void HistoryDb::applySettings(SettingsWidget* gui)
{
    auto* ptr = dynamic_cast<SettingsWidget*>(gui);
}

void HistoryDb::freeSettings(SettingsWidget* gui)
{
    delete gui;
}



} // gui