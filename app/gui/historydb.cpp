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

#include <newsflash/warnpop.h>
#include "historydb.h"
#include "../eventlog.h"
#include "../debug.h"
#include "../historydb.h"

namespace gui
{

HistoryDbSettings::HistoryDbSettings(app::HistoryDb* model) : m_model(model)
{
    m_ui.setupUi(this);
    m_ui.tableView->setModel(m_model);
}

HistoryDbSettings::~HistoryDbSettings()
{}

void HistoryDbSettings::on_btnClear_clicked()
{}

HistoryDb::HistoryDb(app::HistoryDb* model) : m_model(model)
{
    DEBUG("History UI created");
}

HistoryDb::~HistoryDb()
{
    DEBUG("History UI deleted");
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