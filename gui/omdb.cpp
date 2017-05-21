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
    QString apikey = s.get("omdb", "apikey").toString();
    omdb_->setApikey(apikey);
}

void Omdb::saveState(app::Settings& s)
{
    s.set("omdb", "apikey", omdb_->apikey());
}

SettingsWidget* Omdb::getSettings()
{
    return new OmdbSettings(omdb_->apikey());
}

void Omdb::applySettings(SettingsWidget* gui)
{
    const auto* mine = dynamic_cast<OmdbSettings*>(gui);
    const auto& key  = mine->apikey();
    omdb_->setApikey(key);
}

void Omdb::freeSettings(SettingsWidget* s)
{
    delete s;
}

} // namespace