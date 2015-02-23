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
#  include <QObject>
#include <newsflash/warnpop.h>

namespace app
{
    struct Archive;

    class Archiver : public QObject
    {
        Q_OBJECT

    public:
        struct Settings {
            bool keepBroken;
            bool overWriteExisting;
            bool purgeOnSuccess;
        };

        struct Volume {
            QString file;
        };

        struct File {
            QString name;
        };

        ~Archiver() = default;

        // begin extraction of the archive
        virtual void extract(const Archive& arc) = 0;

        // stop the current extraction.
        virtual void stop() = 0;

        // apply the new settings in the archiveer
        virtual void configure(const Settings& settings) = 0;

    signals:
        // list a new file inside the archive.
        void listFile(const app::Archive& arc, const File& file);

        // emit volume information when dealing with archives split
        // into multiple volumes/files.
        void listVolume(const app::Archive& arc, const Volume& vol);

        // emit progress information for the exctration process.
        // file is the current file being extracted from the archive
        // and done is the completion percentage [0, 100]
        void progress(const app::Archive& arc, const File& file, int done);

        // emit this signal when archive extraction completes.
        // this is emitted regardless of extraction success or failure.
        // the archive status should be inspected.
        void ready(const app::Archive& arc);
    protected:
    private:
    };

} // app