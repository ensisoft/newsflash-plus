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

#define LOGTAG "app"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <boost/version.hpp>
#  include <QStringList>
#  include <QString>
#  include <QRegExp>
#include <newsflash/warnpop.h>
#include <zlib/zlib.h>

//#include <Python.h>
#include "eventlog.h"
#include "format.h"

namespace app
{

bool checkVersionUpdate(const QString& current, const QString& latest)
{
    // match version string like "3.3.0b7" where the b7 (beta 7) is optional
    const QRegExp regex("(\\d+).(\\d+).(\\d+)(b(\\d+))?");
    
    if (regex.indexIn(current) == -1)
        return false;

    struct version {
        int major;
        int minor;
        int revision;
        int beta;
    };

    version cur  = {};
    cur.major    = regex.cap(1).toInt();
    cur.minor    = regex.cap(2).toInt();
    cur.revision = regex.cap(3).toInt();
    cur.beta     = regex.cap(5).toInt();
    
    if (regex.indexIn(latest) == -1)
        return false;

    version other  = {};
    other.major    = regex.cap(1).toInt();
    other.minor    = regex.cap(2).toInt();
    other.revision = regex.cap(3).toInt();
    other.beta     = regex.cap(5).toInt();
    
    if (other.major > cur.major)
        return true;
    if (other.major < cur.major)
        return false;
    
    if (other.minor > cur.minor)
        return true;
    if (other.minor < cur.minor)
        return false;
    
    if (other.revision > cur.revision)
        return true;
    if (other.revision < cur.revision)
        return false;
    
    // if beta is 0 it means it's not a beta at all
    // and a non-beta version is newer than beta
    if (cur.beta && other.beta)
        if (other.beta > cur.beta)
            return true;

    if (!cur.beta && !other.beta)
        return false;
    
    if (!cur.beta && other.beta)
        return false;
    
    if (cur.beta && !other.beta)
        return true;
    
    return false;

}

void logCopyright()
{
    const auto boost_major    = BOOST_VERSION / 100000;
    const auto boost_minor    = BOOST_VERSION / 100 % 1000;
    const auto boost_revision = BOOST_VERSION % 100;

    INFO(NEWSFLASH_TITLE " " NEWSFLASH_VERSION);
    INFO(QString::fromUtf8(NEWSFLASH_COPYRIGHT));
    INFO(NEWSFLASH_WEBSITE);
    INFO("Compiled: " __DATE__ ", " __TIME__);    
    INFO(str("Compiler: " COMPILER_NAME ", _1", COMPILER_VERSION));
    INFO(str("Boost software library _1._2._3", boost_major, boost_minor, boost_revision));
    INFO("http://www.boost.org");
    INFO("16x16 Free Application Icons");
    INFO("Copyright (c) 2009 Aha-Soft");
    INFO("http://www.small-icons.com/stock-icons/16x16-free-application-icons.htm");
    INFO("http://www.aha-soft.com");
    INFO("Silk Icon Set 1.3");
    INFO("Copyright (c) Mark James");
    INFO("http://www.famfamfam.com/lab/icons/silk/");
    INFO(str("Qt cross-platform application and UI framework _1", QT_VERSION_STR));
    INFO("http://qt.nokia.com");
    INFO("Zlib compression library " ZLIB_VERSION);
    INFO("Copyright (c) 1995-2010 Jean-Loup Gailly & Mark Adler");
    INFO("http://zlib.net");        

// #ifdef NEWSFLASH_ENABLE_PYTHON
//     const auto pyVersion   = QString(Py_GetVersion()).replace("\n", " ");
//     INFO(str("Python _1", pyVersion));

//     const auto pyCopyright = QString(Py_GetCopyright()).split("\n", QString::SkipEmptyParts);
//     for (int i=0; i<pyCopyright.size(); ++i)
//     {
//         INFO(pyCopyright[i]);
//     }
//     INFO("http://www.python.org");
// #endif
}

} // app