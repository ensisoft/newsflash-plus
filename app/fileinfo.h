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

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QString>
#  include <QMetaType>
#include "newsflash/warnpop.h"

#include "filetype.h"

namespace app
{
    // DataFileInfo describes a new data /media file
    // available in the filesystem.
    struct FileInfo {
        // path to the file
        QString path;

        // name of the file at the path
        QString name;

        // size of the file
        quint64 size;

        // true if apparently damaged.
        bool damaged;

        // true if data is binary.
        bool binary;

        FileType type;
    };

    // FilePackInfo describes a group (a pack) of new files
    // available in the filesystem.
    struct FilePackInfo {
        // path to the batch of files (folder)
        QString path;

        // human readable description
        QString desc;

        quint64 numFiles = 0;

        bool damaged = false;
    };

    struct HeaderInfo {
        QString groupName;

        QString groupPath;

        quint64 numLocalArticles = 0;

        quint64 numRemoteArticles = 0;
    };

    struct HeaderUpdateInfo {
        QString groupName;

        QString groupFile;

        quint64 numLocalArticles = 0;

        quint64 numRemoteArticles = 0;

        quint32 account = 0;

        const void* snapshot = nullptr;
    };

} // app

    Q_DECLARE_METATYPE(app::FileInfo);
    Q_DECLARE_METATYPE(app::FilePackInfo);
    Q_DECLARE_METATYPE(app::HeaderInfo);
    Q_DECLARE_METATYPE(app::HeaderUpdateInfo);


