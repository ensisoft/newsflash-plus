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

#pragma once

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QObject>
#include "newsflash/warnpop.h"

#include <vector>

#include "fileinfo.h"
#include "archive.h"

namespace app
{
    class Reporter  : public QObject
    {
        Q_OBJECT

    public:
        Reporter();
       ~Reporter();

        void enableJournal();
        void cancelJournal();

        bool isJournaling() const
        { return mShouldJournal; }

    signals:
        void sendTextReport(const QString& subject, const QString& message);

    public slots:
        void packCompleted(const app::FilePackInfo& pack);
        void repairReady(const app::Archive& archive);
        void unpackReady(const app::Archive& archive);
        void compileReport();

    private:
        bool mShouldJournal = false;
        bool mReportDownloads = false;
        bool mReportRepairs = false;
        bool mReportUnpacks = false;
    private:
        std::vector<FilePackInfo> mFilePacks;
        std::vector<Archive> mRepairs;
        std::vector<Archive> mUnpacks;

    };

    extern Reporter* g_reporter;

} // namespace
