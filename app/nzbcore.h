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
#  include <QStringList>
#  include <QString>
#  include <QTimer>
#  include <QObject>
#include <newsflash/warnpop.h>
#include <functional>
#include <memory>
#include <queue>
#include <set>

namespace app
{
    class NZBThread;
    class NZBCore : public QObject
    {
        Q_OBJECT
    public:
        enum class PostAction {
            // mark the file as visited
            Rename,

            // delete the file
            Delete
        };

        NZBCore();
       ~NZBCore();

        std::function<bool (const QString& file)> PromptForFile;

        // turn on/off nzb file watching.
        // when watching is turned on the list of folders in the 
        // current watchlist is periodically inspected for new 
        // (unprocessed) .nzb files. 
        void watch(bool on_off);

        // set the list of folders to watch.
        void setWatchFolders(QStringList folders)
        { watchFolders_ = std::move(folders); }

        void setPostAction(PostAction action)
        { action_ = action; }

        void downloadNzbContents(const QString& file, const QString& basePath, const QString& path, const QString& desc,
            quint32 account);

        const QStringList& getWatchFolders() const 
        { return watchFolders_; }

        bool isEnabled() const 
        { return timer_.isActive(); }

        PostAction getAction() const 
        { return action_; }

    private slots:
        void performScan();

    private:
        QStringList watchFolders_;
        QTimer timer_;
    private:
        PostAction action_;
    private:
        std::set<QString> ignored_;
    };
} // app