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
#  include <QProcess>
#include <newsflash/warnpop.h>
#include "archive.h"
#include "archiver.h"

namespace app
{
    // implement extraction of .rar archives using unrar command line utility.
    class Unrar : public Archiver
    {
        Q_OBJECT

    public:
        Unrar();
       ~Unrar();

        virtual void extract(const Archive& arc) override;

        virtual void stop() override;

        virtual void configure(const Settings& settings);

        static 
        bool parseVolume(const QString& line, QString& volume);

        static 
        bool parseProgress(const QString& line, QString& file, int& done);


    private slots:
        void processStdOut();
        void processStdErr();
        void processFinished(int exitCode, QProcess::ExitStatus status);
        void processError(QProcess::ProcessError error);

    private:
        void updateState(const QString& line);

    private:
        QProcess process_;
        QByteArray stdout_;
        QByteArray stderr_;

    private:
        Archive archive_;

    private:
    };

} // ap