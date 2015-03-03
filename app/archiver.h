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
#  include <QObject>
#include <newsflash/warnpop.h>
#include <functional>

namespace app
{
    struct Archive;

    // extract content files from an Archive such as .rar or .zip archive.
    class Archiver
    {
    public:
        struct Settings {
            bool keepBroken;
            bool overWriteExisting;
            bool purgeOnSuccess;
            bool writeLog;
        };


        virtual ~Archiver() = default;

        // begin extraction of the archive
        virtual void extract(const Archive& arc, const Settings& settings) = 0;

        // stop the current extraction.
        virtual void stop() = 0;

        std::function<void (const app::Archive& arc, const QString& file)> onExtract;

        std::function<void (const app::Archive& arc, const QString& target, int done)> onProgress;

        std::function<void (const app::Archive& arc)> onReady;

        virtual bool isRunning() const = 0;

        virtual QStringList findArchives(const QStringList& fileNames) const = 0;

    protected:
    private:
    };

} // app