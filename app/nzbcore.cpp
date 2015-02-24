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

#define LOGTAG "nzb"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>

#include <newsflash/warnpop.h>
#include "nzbcore.h"
#include "debug.h"
#include "eventlog.h"

namespace app
{

NZBCore::NZBCore()
{
    QObject::connect(&watch_timer_, SIGNAL(timeout()),
        this, SLOT(performScan()));

    ppa_ = post_processing_action::mark_processed;

    DEBUG("nzbcore created");
}

NZBCore::~NZBCore()
{
    DEBUG("nzbcore deleted");
}

void NZBCore::watch(bool on_off)
{
    DEBUG("Scanning timer is on %1", on_off);

    if (!on_off)
        watch_timer_.stop();
    else watch_timer_.start();

    watch_timer_.setInterval(10 * 1000);
}

void NZBCore::performScan()
{
    for (int i=0; i<watched_folders_.size(); ++i)
    {

    }
}




} //app