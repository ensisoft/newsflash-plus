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
#  include <QtDebug>
#  include <QTime>
#  include <QString>
#include <newsflash/warnpop.h>

#include <mutex>

namespace debug 
{
    // we use qDebug() for output stream since it supports many
    // of the Qt types and and is thus convenient.

    inline
    QString stamp(const char* file, int line) 
    {
        return QString("%1,%2").arg(file).arg(line);
    }

    std::mutex& get_lock(); 

} // debug

#if defined(NEWSFLASH_DEBUG)
#  define DEBUG(msg) \
     do { \
        std::lock_guard<std::mutex> lock(debug::get_lock()); \
        qDebug() << QTime::currentTime() << debug::stamp(__FILE__, __LINE__) << msg; \
     } while (0)
#else
#  define DEBUG(msg)
#endif

