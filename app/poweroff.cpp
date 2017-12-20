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

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QTimer>
#include "newsflash/warnpop.h"
#include "poweroff.h"
#include "platform.h"
#include "debug.h"
#include "eventlog.h"

namespace app
{

void Poweroff::repairEnqueue()
{
    ++mNumPendingRepairs;
}

void Poweroff::repairReady()
{
    --mNumPendingRepairs;

    evaluate();
}

void Poweroff::unpackEnqueue()
{
    ++mNumPendingUnpacks;
}

void Poweroff::unpackReady()
{
    --mNumPendingUnpacks;

    evaluate();
}

void Poweroff::numPendingArchives(std::size_t num)
{
    mNumPendingArchives = num;

    evaluate();
}

void Poweroff::numPendingTasks(std::size_t num)
{
    mNumPendingTasks = num;

    evaluate();
}

void Poweroff::evaluate()
{
    if (!isPoweroffEnabled())
        return;

    // engine emits multiple pending task signals
    // we're going to ignore copious signals here, i.e.
    // if we're already shutting down then don't do it again.
    if (mShouldPowerOff)
        return;

    //DEBUG("Poweroff wait: downloads %1, repairs, %2, unpacks %3",
    //    mWaitDownloads, mWaitRepairs, mWaitUnpacks);

    if (mWaitDownloads && mNumPendingTasks)
    {
        //DEBUG("Poweroff pending tasks: %1", mNumPendingTasks);
        return;
    }

    if (mWaitRepairs || mWaitUnpacks)
    {
        if (mNumPendingArchives)
        {
            //DEBUG("Poweroff pending archives: %1", mNumPendingArchives);
            return;
        }
    }

    if (mWaitRepairs && mNumPendingRepairs)
    {
        //DEBUG("Poweroff pending repairs: %1", mNumPendingRepairs);
        return;
    }
    if (mWaitUnpacks && mNumPendingUnpacks)
    {
        //DEBUG("Poweroff pending unpacks: %1", mNumPendingUnpacks);
        return;
    }

    DEBUG("Setting up timer to shutdown!");

    mShouldPowerOff = true;

    // instead of emitting the signal immediately
    // and starting the shutdown sequence while *executing*
    // the signal that triggers the shutdown we're just
    // going to schedule the shutdown.
    // this allows the call to return to the main loop
    // and unwind the stack nicely.
    // otherwise we would have a situation where some component
    // emits a signal which calls here to evaluate which
    // calls to initpoweroff which starts the shutdown
    // sequence which calls back into the module which started
    // the shutdown...
    QTimer::singleShot(2, this, SLOT(startShutdown()));

    //emit initPoweroff();
}

void Poweroff::startShutdown()
{
    emit initPoweroff();
}

Poweroff* g_poweroff;

} // app