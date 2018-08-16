// Copyright (c) 2010-2017 Sami Väisänen, Ensisoft
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

#define LOGTAG "omdb"

#include "newsflash/config.h"

#include "app/settings.h"
#include "app/omdb.h"
#include "dlgmovie.h"
#include "omdb.h"

namespace gui
{

void Omdb::loadState(app::Settings& s)
{
    const auto apikey  = s.get("omdb", "apikey").toString();

    // this enabled flag was missing from the original implementation.
    //  so to maintain old logic we assume that if there's an apikey then the flag is
    // enabled.
    bool isEnabled = false;

    if (s.contains("omdb", "enabled"))
    {
        isEnabled = s.get("omdb", "enabled").toBool();
    }
    else
    {
        isEnabled = s.contains("omdb", "apikey");
    }

    omdb_->setApikey(apikey);
    omdb_->setEnabled(isEnabled);
}

void Omdb::saveState(app::Settings& s)
{
    s.set("omdb", "apikey", omdb_->getApikey());
    s.set("omdb", "enabled", omdb_->isEnabled());
}

SettingsWidget* Omdb::getSettings()
{
    return new OmdbSettings(omdb_->getApikey(), omdb_->isEnabled());
}

void Omdb::applySettings(SettingsWidget* gui)
{
    const auto* mine = dynamic_cast<OmdbSettings*>(gui);
    const auto& key  = mine->apikey();
    const auto& enabled = mine->isEnabled();
    omdb_->setApikey(key);
    omdb_->setEnabled(enabled);
}

void Omdb::freeSettings(SettingsWidget* s)
{
    delete s;
}

} // namespace