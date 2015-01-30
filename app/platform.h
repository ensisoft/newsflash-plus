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
#  include <QtGui/QIcon>
#  include <QString>
#include <newsflash/warnpush.h>


// platform specific functions that are not provided by Qt.

namespace app
{

// extract appliation icon from a 3rd party executable specified
// by binary. binary is expected to be the complete path to the
// executable in question.
QIcon extract_icon(const QString& binary);

// return the name of the operating system that we're running on
// for example Mint Linux, Ubuntu, Windows XP, Windows 7  etc.
QString get_platform_name();

// resolve the directory path to a mount-point / disk 
QString resolve_mount_point(const QString& directory);

// get free space available on the disk that contains
// the object identified by filename
quint64 get_free_disk_space(const QString& filename);

// open a file on the local computer
void open_file(const QString& file);

void open_web(const QString& url);

// perform computer shutdown.
void shutdown_computer();

#if defined(LINUX_OS)

    void set_openfile_command(const QString& cmd);

    void set_shutdown_command(const QString& cmd);

    QString get_openfile_command();

    QString get_shutdown_command();
#endif

} // app