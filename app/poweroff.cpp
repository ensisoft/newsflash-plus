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

#define LOGTAG "power"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>

#include <newsflash/warnpop.h>
#include "poweroff.h"
#include "platform.h"
#include "debug.h"
#include "eventlog.h"

namespace app
{

Poweroff::Poweroff() : numRepairs_(0), numUnpacks_(0), numArchives_(0), numTasks_(0)
{
   waitDownloads_ = false;
   waitRepairs_   = false;
   waitUnpacks_   = false;
   powerOff_      = false;
}

void Poweroff::repairEnqueue()
{
    ++numRepairs_;
}

void Poweroff::repairReady()
{
    --numRepairs_;    

    evaluate();
}

void Poweroff::unpackEnqueue()
{
    ++numUnpacks_;
}

void Poweroff::unpackReady()
{
    --numUnpacks_;

    evaluate();
}

void Poweroff::numPendingArchives(std::size_t num)
{
    numArchives_ = num;

    evaluate();
}

void Poweroff::numPendingTasks(std::size_t num)
{
    numTasks_ = num;

    evaluate();
}

void Poweroff::evaluate()
{
    if (!waitDownloads_ && !waitRepairs_ && !waitUnpacks_)
        return;

    if (waitDownloads_ && numTasks_)
        return;
    if (waitRepairs_ || waitUnpacks_)
    {
        if (numArchives_)
            return;
    }

    if (waitRepairs_ && numRepairs_)
        return;
    if (waitUnpacks_ && numUnpacks_)
        return;

    powerOff_ = true;
    emit initPoweroff();
}

Poweroff* g_poweroff;

} // app