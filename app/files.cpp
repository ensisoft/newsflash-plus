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

#define LOGTAG "files"

#include <newsflash/config.h>

#include "eventlog.h"
#include "debug.h"
#include "engine.h"
#include "files.h"

namespace {
    enum class columns {
        type, time, path, name, count
    };
}

namespace app
{

files::files()
{
    QObject::connect(g_engine, SIGNAL(fileCompleted(const app::file&)),
        this, SLOT(fileCompleted(const app::file&)));

    DEBUG("files model created");
}

files::~files()
{
    DEBUG("files model deleted");
}

QVariant files::data(const QModelIndex& index, int role) const
{
    return QVariant();
}

QVariant files::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch ((columns)section)
        {
            case columns::type:  return "Type";
            case columns::time:  return "Time";
            case columns::path:  return "Location";
            case columns::name:  return "Name";
            case columns::count:
                Q_ASSERT(0);
        }
    }
    return QVariant();
}

void files::sort(int column, Qt::SortOrder order) 
{

}

int files::rowCount(const QModelIndex&) const 
{ 
    return (int)files_.size();
}

int files::columnCount(const QModelIndex&) const 
{
    return (int)columns::count;
}

void files::fileCompleted(const app::file& file)
{

}

} // app
