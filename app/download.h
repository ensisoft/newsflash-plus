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

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QString>
#  include <QMetaType>
#  include <QDateTime>
#include <newsflash/warnpop.h>
#include "media.h"

namespace app
{
    // Download bundle properties.
    struct Download {

        // Assumed type of the media. 
        MediaType type;

        // Source where this download data comes from.
        MediaSource source;

        // The account to be used for the download.
        quint32 account;

        // The basepath in the file system where the content
        // is to be placed. If this is not empty this will override
        // the default location defined in the engine.
        QString basepath;

        // The name of the folder to be created for all the content items 
        // the download. If this is empty then data files are stored in 
        // basePath location.
        QString folder;

        // The human readable description for the download.
        QString desc;

        Download() {
            static quint32 id = 1;
            guid = id++;
        }

        quint32 getGuid() const 
        { return guid; }
        
    private:
        quint32 guid;
    };

} // app

    Q_DECLARE_METATYPE(app::Download);