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

#define LOGTAG "tasklist"

#include <newsflash/config.h>

#include "tasklist.h"

namespace app
{

tasklist::tasklist()
{}

tasklist::~tasklist()
{}

QVariant tasklist::data(const QModelIndex& index, int role) const 
{
    return QVariant();
}

QVariant tasklist::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (columns(section))
    {
        case columns::status:
            return "Status";
        case columns::priority:
            return "Priority";
        case columns::done:
            return "Done";
        case columns::time:
            return "Time";
        case columns::eta:
            return "ETA";
        case columns::size:
            return "Size";
        case columns::desc:
            return "Description";
        default:
            Q_ASSERT(!"incorrect column");
            break;
    }    
    return QVariant();
}

int tasklist::rowCount(const QModelIndex&) const 
{
    return 0;
}

int tasklist::columnCount(const QModelIndex&) const
{
    return (int)columns::sentinel;
}

} // app
