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
#include "newsflash/warnpop.h"

#include <functional>
#include <set>

#include "archive.h"
#include "archiver.h"
#include "process.h"

namespace app
{
    // implement extraction of .rar archives using unrar command line utility.
    class Unrar : public Archiver
    {
    public:
        Unrar(const QString& executable);
       ~Unrar();

        // Archiever implementation.
        virtual void extract(const Archive& arc, const Settings& settings) override;
        virtual void stop() override;
        virtual bool isRunning() const override;
        virtual QStringList findArchives(const QStringList& fileNames) const override;
        virtual bool isSupportedFormat(const QString& filePath, const QString& fileName) const override;
        virtual bool hasProgressInfo() const override
        { return true; }
        virtual bool getCurrentArchiveData(Archive* arc) const override;

        // public static parse functions for easy unit testing.
        static bool parseMessage(const QString& line, QString& msg);
        static bool parseVolume(const QString& line, QString& volume);
        static bool parseProgress(const QString& line, QString& file, int& done);
        static bool parseTermination(const QString& line);

        static QStringList findVolumes(const QStringList& files);
        static QString getCopyright(const QString& executable);

    private:
        void onFinished();
        void onStdErr(const QString& line);
        void onStdOut(const QString& line);

    private:
        Process mProcess;
        QString mUnrarExecutable;
    private:
        Archive mCurrentArchive;
        QString mMessage;
        QString mErrors;
    private:
        std::set<QString> mCleanupSet;
        bool mDoCleanup = false;
    };

} // ap