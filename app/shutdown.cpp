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

#define LOGTAG "shutdown"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>

#include <newsflash/warnpop.h>
#include "shutdown.h"
#include "platform.h"
#include "debug.h"
#include "eventlog.h"

namespace app
{

Shutdown::Shutdown() : numRepairs_(0), numUnpacks_(0), numDownloads_(0)
{
   waitDownloads_ = false;
   waitRepairs_   = false;
   waitUnpacks_   = false;
   powerOff_      = false;
}

void Shutdown::repairEnqueue()
{
    ++numRepairs_;

    DEBUG("Pending repairs %1", numRepairs_);
}

void Shutdown::repairReady()
{
    --numRepairs_;    

    evaluate();
}

void Shutdown::unpackEnqueue()
{
    ++numUnpacks_;

    DEBUG("Pending unpacks %1", numUnpacks_);
}

void Shutdown::unpackReady()
{
    --numUnpacks_;

    evaluate();
}

void Shutdown::evaluate()
{
    if (waitDownloads_ && numDownloads_)
        return;
    if (waitRepairs_ && numRepairs_)
        return;
    if (waitUnpacks_ && numUnpacks_)
        return;

    powerOff_ = true;
    emit initPoweroff();
}

Shutdown* g_shutdown;

} // app