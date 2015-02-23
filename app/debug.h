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
#  include <QtNetwork/QNetworkReply>
#  include <QtDebug>
#  include <QTime>
#  include <QString>
#  include <QProcess>
#  include <QFile>
#include <newsflash/warnpop.h>
#include <mutex>

namespace debug
{

    // debug formatting is aimed for debugging purposes only.
    // this might produce different results than app::format which 
    // is for user interfacing.

    // Qt already provides 2 mechanisms to do output.
    // 1. qDebug(const char* msg, ...)
    // 2. qDebug() << "Brush" << myBrush.
    // 
    // Option 1. is concise but unsafe (only C types allowed through ellipsis)
    // Option 2. is safe but verbose and ugly to use.
    // 
    // Ideally we want to combine the best of both worlds into one solution
    // that gives us a simple syntax like DEBUG("My Brush %1 bla bla", myBrush);
    // and allows us to easily extend and customize it (through ADL and overloading)

    inline QString stamp(const char* file, int line) 
    {
        return QString("%1,%2").arg(file).arg(line);
    }

    // get global mutex lock for output locking (todo: is this needed?)
    std::mutex& getLock(); 


    QString toString(QProcess::ProcessError error);
    QString toString(QProcess::ExitStatus status);
    QString toString(QNetworkReply::NetworkError error);
    QString toString(QFile::FileError error);
    QString toString(const QNetworkReply& reply);

    inline QString toString(bool value)
    { return (value ? "True" : "False"); }

    inline QString toString(const char* str)
    { return str; }

    inline QString toString(const QString& str)
    { return str; }

    template<typename T>
    QString toString(const T& value)
    {
        QString str;
        QDebug dbg(&str);
        dbg << value;
        return str.trimmed(); 
    }

    template<typename T, typename... Args>
    QString toString(QString fmt, const T& value, const Args&... args)
    {
        fmt = fmt.arg(toString(value));
        return toString(fmt, args...);
    }

} // debug

#if defined(NEWSFLASH_DEBUG)
#  define DEBUG(fmt, ...) \
     do { \
        std::lock_guard<std::mutex> lock(debug::getLock()); \
        qDebug() << QTime::currentTime() << debug::stamp(__FILE__, __LINE__) << debug::toString(fmt, ## __VA_ARGS__); \
     } while (0)
#else
#  define DEBUG(msg, ...)
#endif

#define BOUNDSCHECK(container, index) Q_ASSERT(index < container.size())