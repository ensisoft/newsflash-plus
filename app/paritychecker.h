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
    struct Recovery;

    class ParityChecker : public QObject
    {
        Q_OBJECT

    public:
        struct Settings {
            bool writeLogFile;
            bool purgeOnSuccess;
        };

        enum class FileState {

        };

        enum class FileType {
            Target, Parity
        };

        // information about a file referenced in the 
        // parity checking process. could be either a target file
        // or a checksum/data file
        struct File {
            FileState file;
            FileType  type;
            QString   name;
        };

       ~ParityChecker() = default;

        virtual void recover(const Recovery& rec) = 0;

        virtual void stop() = 0;

        virtual void configure(const Settings& settings) = 0;

    signals:
        // list a new file in the parity process.
        void listFile(const app::Recovery& rec, const File& file);

        void scanProgress(const app::Recovery& rec, const File& file, float done);

        void repairProgress(const QString& step, float done);

        void ready(const app::Recovery& rec);  
    protected:
    private:      
    };

} // app