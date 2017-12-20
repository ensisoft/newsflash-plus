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

#define LOGTAG "reporter"

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QStringList>
#include "newsflash/warnpop.h"

#include "fileinfo.h"
#include "archive.h"
#include "debug.h"
#include "eventlog.h"
#include "reporter.h"

namespace app
{

Reporter* g_reporter;

Reporter::Reporter()
{
    DEBUG("Reporter created");
}

Reporter::~Reporter()
{
    DEBUG("Reporter destroyed");
}

void Reporter::cancelJournal()
{
    mShouldJournal = false;
    mFilePacks.clear();
    mRepairs.clear();
    mUnpacks.clear();
}

void Reporter::enableJournal()
{
    mShouldJournal = true;
}

// slot
void Reporter::packCompleted(const app::FilePackInfo& pack)
{
    mFilePacks.push_back(pack);
}

// slot
void Reporter::repairReady(const app::Archive& archive)
{
    mRepairs.push_back(archive);
}

// slot
void Reporter::unpackReady(const app::Archive& archive)
{
    mUnpacks.push_back(archive);
}

// slot
void Reporter::compileReport()
{
    QStringList message;
    message << "Hello,\r\n\r\nbelow are the details for your last night's downloads.\r\n";

    for (const auto& pack : mFilePacks)
    {
        if (pack.damaged)
            message << toString("The file batch (%1) (%2 files) completed succesfully.\r\n",
                pack.path, pack.numFiles);
        else
            message << toString("The file batch (%1) (%2 files) completed with errors.\r\n",
                pack.path, pack.numFiles);
    }

    for (const auto& repair : mRepairs)
    {
        if (repair.state == Archive::Status::Success)
            message << toString("The archive %1/%2 was repaired succesfully.\r\n",
                repair.path, repair.file);
        else if (repair.state == Archive::Status::Error)
            message << toString("The archive %1/2 failed to repair. (%3).\r\n",
                repair.path, repair.file, repair.message);
    }

    for (const auto& unpack : mUnpacks)
    {
        if (unpack.state == Archive::Status::Success)
            message << toString("The archive %1/%2 was extracted succesfully.\r\n",
                unpack.path, unpack.file);
        else if (unpack.state == Archive::Status::Error)
            message << toString("The archive %1/%2 failed to extra. (%3).\r\n",
                unpack.path, unpack.file, unpack.message);
    }

    message << "--";
    message << "That's all. Have a good day! :)";

    const auto& subject = toString("%1 Download Report", NEWSFLASH_TITLE);
    const auto& content = message.join("\r\n");

    emit sendTextReport(subject, content);
}

} // namespace
