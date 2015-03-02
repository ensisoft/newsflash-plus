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

#define LOGTAG "arcman"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QStringList>
#  include <QString>
#  include <QFileInfo>
#  include <QDir>
#include <newsflash/warnpop.h>
#include "debug.h"
#include "arcman.h"
#include "archiver.h"
#include "fileinfo.h"
#include "repairer.h"
#include "unpacker.h"


namespace app
{

ArchiveManager::ArchiveManager(Repairer& repairer, Unpacker& unpacker) : repairer_(repairer), unpacker_(unpacker)
{
    DEBUG("ArchiveManager created");

    QObject::connect(&repairer, SIGNAL(repairReady(const app::Archive&)),
        this, SLOT(repairReady(const app::Archive&)));
    QObject::connect(&unpacker, SIGNAL(unpackReady(const app::Archive&)),
        this, SLOT(unpackReady(const app::Archive&)));
}

ArchiveManager::~ArchiveManager()
{
    DEBUG("ArchiveManager deleted");
}

void ArchiveManager::fileCompleted(const app::FileInfo& file)
{}

void ArchiveManager::packCompleted(const app::FilePackInfo& pack)
{
    DEBUG("FilePack ready %1 %2", pack.desc, pack.path);

    static const char* attempts[] = {

    };

    QStringList files;

    QDir dir;
    dir.setPath(pack.path);
    dir.setNameFilters(QStringList("*.par2"));
    files = dir.entryList();
    for (int i=0; i<files.size(); ++i)
    {
        QFileInfo file(files[i]);
        QString name = file.completeBaseName();
        if (name.contains(".vol"))
            continue;

        Archive arc;
        arc.path  = pack.path;
        arc.desc  = file.fileName();
        arc.file  = file.fileName();        
        arc.state = Archive::Status::Queued;
        repairer_.addRecovery(arc);
    }
}

void ArchiveManager::repairReady(const app::Archive& arc)
{
    QDir dir;
    dir.setPath(arc.path);
    const auto& entries = dir.entryList();
    const auto& volumes = unpacker_.findUnpackVolumes(entries);
    for (const auto& vol : volumes)
    {
        if (unpacks_.find(arc.path + "/" + vol) != std::end(unpacks_))
            continue;

        Archive unrar;
        unrar.path  = arc.path;
        unrar.file  = vol;
        unrar.desc  = vol;
        unrar.state = Archive::Status::Queued;
        unpacker_.addUnpack(unrar);
        unpacks_.insert(arc.path + "/" + vol);
    }
}

void ArchiveManager::unpackReady(const app::Archive& arc)
{
    QDir dir;
    dir.setPath(arc.path);
    const auto& entries = dir.entryList();
    const auto& volumes = unpacker_.findUnpackVolumes(entries);
    for (const auto& vol : volumes)
    {
        if (unpacks_.find(arc.path + "/" + vol) != std::end(unpacks_))
            continue;

        Archive unrar;
        unrar.path = arc.path;
        unrar.file = vol;
        unrar.desc = vol;
        unrar.state = Archive::Status::Queued;
        unpacks_.insert(arc.path + "/" + vol);
    }
}

} // app