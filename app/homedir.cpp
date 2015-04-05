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
    if (!dir.mkpath(mine))
        throw std::runtime_error(narrow(toString("failed to create %1", mine)));

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

QString homedir::path(const QString& name)
{
    const auto base = pathstr;
    const auto native = QDir::toNativeSeparators(base + "/" + name + "/");
    return native;
}

} // app