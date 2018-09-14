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
#  include <QtGui/QIcon>
#  include <QString>
#  include <QMetaType>
#include "newsflash/warnpop.h"

namespace app
{
    // File archive such as .zip, .rar etc.
    struct Archive {
        enum class Status {
            // archive has no status (reset)
            None,

            // archive is queued and will execute at some later time.
            Queued,

            // archive is currently being unpacked.
            Active,

            // archive was succesfully unpacked.
            Success,

            // archive failed to unpack
            Failed,

            // error running the unpack operation.
            Error,

            // stopped/canceled by the user.
            Stopped
        };

        // current unpack state of the archive.
        Status state = Status::None;

        // path to the unpack source folder (where the .rar files are)
        QString path;

        // the main .rar file
        QString file;

        // human readable description.
        QString desc;

        // error/information message
        QString message;

        Archive();

        quint32 getGuid() const
        { return guid; }

        void clear()
        {
            guid  = 0;
            state = Status::None;
            path.clear();
            file.clear();
            desc.clear();
            message.clear();
        }

        bool isValid() const
        { return state != Status::None; }

    private:
        quint32 guid = 0;
    };

    QString toString(Archive::Status status);

    QIcon toIcon(Archive::Status status);

} // app

    Q_DECLARE_METATYPE(app::Archive);
