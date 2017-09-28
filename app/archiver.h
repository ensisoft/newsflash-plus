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

    // Archiver encapsulates the details and mechanics for invoking some
    // particular unarchiver tool such as unrar or 7za
    class Archiver
    {
    public:
        struct Settings {
            bool keepBroken = true;
            bool overWriteExisting = false;
            bool purgeOnSuccess = false;
            bool writeLog = true;
        };


        virtual ~Archiver() = default;

        // Begin extraction of the archive.
        virtual void extract(const Archive& arc, const Settings& settings) = 0;

        // Stop the current extraction (if any).
        virtual void stop() = 0;

        // This callback is invoked to indicate the file being extracted.
        std::function<void (const app::Archive& arc, const QString& file)> onExtract;

        // This callback is invoked to indicate progress on some file extraction.
        std::function<void (const app::Archive& arc, const QString& target, int done)> onProgress;

        // This callback is invoked when extraction is ready.
        std::function<void (const app::Archive& arc)> onReady;

        // Returns true if the engine process is currently running otherwise false.
        virtual bool isRunning() const = 0;

        // Given a list of file names return the list of file names that this
        // tool supports and can extract.
        virtual QStringList findArchives(const QStringList& fileNames) const = 0;

        // Returns true if the engine supports the archive format for the given filename.
        virtual bool isSupportedFormat(const QString& filePath, const QString& fileName) const = 0;

        // Returns true if the engine supports reporting progress information.
        virtual bool hasProgressInfo() const = 0;

    protected:
    private:
    };

} // app
