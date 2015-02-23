// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
//
// http://www.ensisoft.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#pragma once

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QStringList>
#  include <QTimer>
#  include <QObject>
#include <newsflash/warnpop.h>
#include <functional>

namespace app
{
    class nzbcore : public QObject
    {
        Q_OBJECT
    public:
        enum class post_processing_action {
            mark_processed,
            delete_file,
            move_to_another_location
        };

        nzbcore();
       ~nzbcore();

        std::function<bool (const QString& file)> on_file;

        // turn on/off nzb file watching.
        // when watching is turned on the list of folders in the 
        // current watchlist is periodically inspected for new 
        // (unprocessed) .nzb files. 
        void watch(bool on_off);

        // set the list of folders to watch.
        void set_watch_folders(const QStringList& folders)
        { watched_folders_ = folders; }

        // set the custom download folder.
        void set_download_folder(const QString& f)
        { download_folder_ = f; }

    private slots:
        void performScan();

    private:
        QStringList watched_folders_;
        QString nzb_dump_folder_;
        QString download_folder_;
        QTimer watch_timer_;
    private:
        post_processing_action ppa_;
    };
} // app