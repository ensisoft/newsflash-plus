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
#  include <QtGlobal> // for quint64
#include <newsflash/warnpush.h>

class QString;
class QPixmap;
class QIcon;

// platform specific functions that are not provided by Qt
// and some extra utility functions.

namespace app
{

QPixmap toGrayScale(const QPixmap& p);

QPixmap toGrayScale(const QString& pixmap);

// extract appliation icon from a 3rd party executable specified
// by binary. binary is expected to be the complete path to the
// executable in question.
QIcon extractIcon(const QString& binary);

// return the name of the operating system that we're running on
// for example Mint Linux, Ubuntu, Windows XP, Windows 7  etc.
QString getPlatformName();

// resolve the directory path to a mount-point / disk 
QString resolveMountPoint(const QString& directory);

// get free space available on the disk that contains
// the object identified by filename
quint64 getFreeDiskSpace(const QString& filename);

// open a file on the local computer
void openFile(const QString& file);

void openWeb(const QString& url);

// perform computer shutdown.
void shutdownComputer();

#if defined(LINUX_OS)
  void setOpenfileCommand(const QString& cmd);
  void setShutdownCommand(const QString& cmd);
  QString getOpenfileCommand();
  QString getShutdownCommand();
#endif

} // app