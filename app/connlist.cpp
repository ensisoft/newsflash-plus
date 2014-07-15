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

#include <newsflash/config.h>

#include "connlist.h"
#include "mainapp.h"

namespace app
{

connlist::connlist()
{}

connlist::~connlist()
{}

QVariant connlist::data(const QModelIndex& index, int role) const
{
    return QVariant();
}

QVariant connlist::headerData(int section, Qt::Orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (columns(section))
    {
        case columns::status:
            return "Status";
        case columns::server:
            return "Server";
        case columns::data:
            return "Data";
        case columns::kbs:
            return "Speed";
        case columns::desc:
            return "Description";
        default:
            Q_ASSERT(!"incorrect column");
            break;
    }
    return QVariant();
}

int connlist::rowCount(const QModelIndex&) const
{
    return 0;
}

int connlist::columnCount(const QModelIndex&) const
{
    return (int)columns::sentinel;
}

} // app
