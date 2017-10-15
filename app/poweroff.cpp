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
    if (!mWaitDownloads && !mWaitRepairs && !mWaitUnpacks)
        return;

    if (mWaitDownloads && mNumPendingTasks)
        return;
    if (mWaitRepairs || mWaitUnpacks)
    {
        if (mNumPendingArchives)
            return;
    }

    if (mWaitRepairs && mNumPendingRepairs)
        return;
    if (mWaitUnpacks && mNumPendingUnpacks)
        return;

    mShouldPowerOff = true;
    emit initPoweroff();
}

Poweroff* g_poweroff;

} // app