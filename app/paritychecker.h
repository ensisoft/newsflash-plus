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

    // paritychecker performs checking on a pack of files.
    // paritychecker can errors in the target files being checked
    // given a sufficient amount of parity (.par2) files.
    class ParityChecker
    {
    public:
        enum class FileState {
            // parity file is being loaded.
            Loading,

            // parity file is loaded.
            Loaded,

            // target file is being scanned.
            Scanning,

            // Target file is missing.
            Missing,

            // Target file is found.
            Found,

            // Target file is empty.
            Empty,

            // Target file is damaged
            Damaged,

            // Target file is complete.
            Complete
        };

        enum class FileType {
            Target, Parity
        };

        // information about a file referenced in the
        // parity checking process. could be either a target file
        // or a checksum/data file
        struct File {
            FileState state;
            FileType  type;
            QString   name;
        };

        struct Settings {
            bool writeLogFile = false;
            bool purgeOnSuccess = false;
        };

        // list a new file in the parity process.
        // this signal is emitted when the file is first encountered
        // and when if the state of an existing file changes.
        std::function<void (const app::Archive& arc, File file)> onUpdateFile;

        std::function<void (const app::Archive& arc, QString file, int done)> onScanProgress;

        std::function<void (const app::Archive& arc, QString step, int done)> onRepairProgress;

        std::function<void (const app::Archive& arc)> onReady;

        virtual ~ParityChecker() = default;

        // start a new recovery.
        virtual void recover(const Archive& arc, const Settings& s) = 0;

        // stop the current recovery process.
        virtual void stop() = 0;

        virtual bool isRunning() const = 0;

        // get the current archive contents if any and returns true
        // otherwise false.
        virtual bool getCurrentArchiveData(Archive* arc) const = 0;
    protected:
    private:
    };

} // app
