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
#  include <QString>
#include <newsflash/warnpop.h>
#include <memory>
#include <vector>
#include <set>
#include <map>
#include "archive.h"

namespace app
{
    struct FileInfo;
    struct FilePackInfo;
    class Repairer;
    class Unpacker;
    class Shutdown;

    // translates file events into archives (when necessary)
    // and manages a list of currently pending archives. 
    // finally executes the list of tools on the archive.
    class ArchiveManager : public QObject
    {
        Q_OBJECT

    public:
        ArchiveManager(Repairer& repairer, Unpacker& unpacker);
       ~ArchiveManager();

    signals:
        void numPendingArchives(std::size_t num);

    public slots:
        void fileCompleted(const app::FileInfo& file);
        void packCompleted(const app::FilePackInfo& pack);

    private slots:
        void repairReady(const app::Archive& arc);
        void unpackReady(const app::Archive& arc);

    private:
        Repairer& m_repairer;
        Unpacker& m_unpacker;
    private:
        std::set<QString> m_unpacks;
        std::map<QString, quint32> m_repairs;

        std::set<quint32> m_pendingArchives;
    };

} // app
