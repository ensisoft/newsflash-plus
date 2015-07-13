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
#  include <QDir>
#  include <QStringList>
#  include <QFile>
#include <newsflash/warnpop.h>
#include "nzbcore.h"
#include "nzbthread.h"
#include "debug.h"
#include "eventlog.h"
#include "engine.h"

namespace app
{

NZBCore::NZBCore() : action_(PostAction::Rename)
{
    QObject::connect(&timer_, SIGNAL(timeout()),
        this, SLOT(performScan()));

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
        timer_.stop();
    else timer_.start();

    timer_.setInterval(10 * 1000);
}

bool NZBCore::downloadNzbContents(const QString& file, const QString& basePath, const QString& path, const QString& desc,
    quint32 account)
{
    QFile io(file);
    if (!io.open(QIODevice::ReadOnly))
    {
        ERROR("Failed to open %1", file, " %2", io.errorString());
        return false;
    }

    QByteArray nzb = io.readAll();
    DEBUG("Read %1 bytes", nzb.size());

    io.close();

    return g_engine->downloadNzbContents(account, basePath, path, desc, nzb);
}

void NZBCore::postProcess(const QString& file)
{
    switch (action_)
    {
        case PostAction::Rename:
            QFile::rename(file, file + "_remove");
            break;

        case PostAction::Delete:
            QFile::remove(file);
            break;
    }
}

void NZBCore::performScan()
{
    timer_.blockSignals(true);
    timer_.stop();

    for (const auto& folder : watchFolders_)
    {
        DEBUG("Scanning folder %1", folder);
        QDir dir;
        dir.setPath(folder);
        dir.setNameFilters(QStringList("*.nzb"));
        const auto files = dir.entryList();
        for (const auto& file : files)
        {
            // check if alredy processed
            if (file.endsWith("_remove"))
                continue;

            const auto absPath = dir.absoluteFilePath(file);
            if (ignored_.find(absPath) != std::end(ignored_))
                continue;

            if (!PromptForFile(absPath))
            {
                ignored_.insert(absPath);
                INFO("%1 is from now on ignored", absPath);
            }
        }
    }
    timer_.blockSignals(false);
    timer_.start();
}

} //app