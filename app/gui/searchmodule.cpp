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

#define LOGTAG "search"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#include <newsflash/warnpop.h>
#include "searchmodule.h"
#include "search.h"
#include "dlgnewznab.h"
#include "../settings.h"
#include "../debug.h"
#include "../newznab.h"

namespace gui
{

SearchSettings::SearchSettings()
{
    ui_.setupUi(this);
}

SearchSettings::~SearchSettings()
{}

bool SearchSettings::validate() const 
{
    return true;
}

void SearchSettings::on_btnImport_clicked()
{}

void SearchSettings::on_btnAdd_clicked()
{
    app::Newznab::Account acc;

    DlgNewznab dlg(this, acc);
    if (dlg.exec() == QDialog::Rejected)
        return;
}

void SearchSettings::on_btnDel_clicked()
{}

void SearchSettings::on_btnEdit_clicked()
{

}

SearchModule::SearchModule()
{
    DEBUG("SearchModule UI created");
}

SearchModule::~SearchModule()
{
    DEBUG("SearchModule UI deleted");
}

void SearchModule::saveState(app::Settings& settings)
{

}

void SearchModule::loadState(app::Settings& settings)
{

}

MainWidget* SearchModule::openSearch()
{
    Search* s = new Search;
    return s;
}

SettingsWidget* SearchModule::getSettings()
{
    auto* p = new SearchSettings();
    return p;
}

void SearchModule::applySettings(SettingsWidget* gui)
{
    auto* p = static_cast<SearchSettings*>(gui);
}

void SearchModule::freeSettings(SettingsWidget* gui)
{
    delete gui;
}

} // gui