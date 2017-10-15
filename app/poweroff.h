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

#pragma once

#include <newsflash/warnpush.h>
#  include <QObject>
#include <newsflash/warnpop.h>

namespace app
{
    class Poweroff : public QObject
    {
        Q_OBJECT

    public:
        void waitDownloads(bool onOff)
        { mWaitDownloads = onOff; }

        void waitRepairs(bool onOff)
        { mWaitRepairs = onOff; }

        void waitUnpacks(bool onOff)
        { mWaitUnpacks = onOff; }

        bool waitDownloads() const
        { return mWaitDownloads; }

        bool waitRepairs() const
        { return mWaitRepairs; }

        bool waitUnpacks() const
        { return mWaitUnpacks; }

        bool shouldPowerOff() const
        { return mShouldPowerOff; }

        bool isPoweroffEnabled() const
        {
            // if any of these is enabled then we're powering off
            // at some point.
            return mWaitDownloads ||
                   mWaitRepairs   ||
                   mWaitUnpacks;

        }

    signals:
        void initPoweroff();

    public slots:
        void repairEnqueue();
        void repairReady();
        void unpackEnqueue();
        void unpackReady();
        void numPendingArchives(std::size_t);
        void numPendingTasks(std::size_t);
    private:
        void evaluate();

    private:
        std::size_t mNumPendingRepairs  = 0;
        std::size_t mNumPendingUnpacks  = 0;
        std::size_t mNumPendingArchives = 0;
        std::size_t mNumPendingTasks    = 0;
    private:
        bool mWaitDownloads  = false;
        bool mWaitRepairs    = false;
        bool mWaitUnpacks    = false;
        bool mShouldPowerOff = false;
    };

    extern Poweroff* g_poweroff;

} // app