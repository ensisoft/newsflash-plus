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

#include <newsflash/warnpush.h>
#  include <QDir>
#include <newsflash/warnpop.h>

#include <stdexcept>
#include "homedir.h"
#include "format.h"

namespace app
{

QString homedir::pathstr;

void homedir::init(const QString& folder)
{
    const auto& home = QDir::homePath();
    const auto& mine = home + "/" + folder;

    QDir dir;
    if (!dir.mkpath(home + "/" + folder))
        throw std::runtime_error(str_a("failed to create _1", mine));

    pathstr = mine;
}

QString homedir::path() 
{
    return pathstr;
}

QString homedir::file(const QString& name)
{
    // pathstr is an absolute path so then this is also
    // an absolute path.
    return QDir::toNativeSeparators(QDir::cleanPath(pathstr + "/" + name));
}

} // app