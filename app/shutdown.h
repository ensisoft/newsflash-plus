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
    class Archive;

    class Shutdown : public QObject
    {
        Q_OBJECT

    public:
        Shutdown();

        void waitDownloads(bool onOff)
        { waitDownloads_ = onOff; }

        void waitRepairs(bool onOff)
        { waitRepairs_ = onOff; }

        void waitUnpacks(bool onOff)
        { waitUnpacks_ = onOff; }

        bool waitDownloads() const 
        { return waitDownloads_; }

        bool waitRepairs() const
        { return waitRepairs_; }

        bool waitUnpacks() const 
        { return waitUnpacks_; }

        bool isPoweroffEnabled() const
        { return powerOff_; }

    signals:
        void initPoweroff();

    public slots:
        void repairEnqueue();
        void repairReady();
        void unpackEnqueue();
        void unpackReady();
    private:
        void evaluate();

    private:
        std::size_t numRepairs_;
        std::size_t numUnpacks_;
        std::size_t numDownloads_;
    private:
        bool waitDownloads_;
        bool waitRepairs_;
        bool waitUnpacks_;
        bool powerOff_;
    };

    extern Shutdown* g_shutdown;

} // app