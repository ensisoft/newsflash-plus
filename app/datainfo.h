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

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QString>
#  include <QMetaType>
#include <newsflash/warnpop.h>
#include "filetype.h"

namespace app
{
    // DataFileInfo describes a new data /media file
    // available in the filesystem.
    struct DataFileInfo {
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

    // new batch of files is available in the filesystem
    struct FileBatchInfo {
        // path to the batch of files (folder)
        QString path;

        // human readable description
        QString desc;

    };

    // newsgroup message notifies of a group information
    // as part of a listing event.
    struct NewsGroupInfo {
        // the estimated number of articles available in the group
        quint64 size;

        // the first article number
        quint64 first;

        // the last article number
        quint64 last;

        // group name
        QString name;
    };    

} // app

    Q_DECLARE_METATYPE(app::DataFileInfo);
    Q_DECLARE_METATYPE(app::NewsGroupInfo);
    Q_DECLARE_METATYPE(app::FileBatchInfo);
