// Copyright (c) 2014 Sami Väisänen, Ensisoft 
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

#include "datastore.h"
#include "groups.h"

namespace app
{

groups::groups()
{}

groups::~groups()
{}

void groups::save(app::datastore& values) const
{

}


void groups::load(const datastore& values) 
{

}

QAbstractItemModel* groups::view() 
{
    return this;
}

QVariant groups::headerData(int section, Qt::Orientation orietantation, int role) const 
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orietantation != Qt::Horizontal)
        return QVariant();

    switch ((groups::column)section)
    {
        case column::name:
            return "Name";
        case column::created:
            return "Created";
        case column::updated:
            return "Updated";
        case column::articles:
            return "Articles";
        case column::size:
            return "Size";
        default:
            Q_ASSERT(!"missing column case");
            break;
    }
    return QVariant();
}

QVariant groups::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        const auto col   = index.column();
        const auto row   = index.row();
        const auto group = groups_[row];
        switch ((groups::column)col)
        {
            case column::name:
               return group.name;
            case column::created:
               return "todo:";
            case column::updated:
                return "todo";
            case column::size:
                return "todo";
            case column::articles:
                return group.size;
            default:
                Q_ASSERT(!"missing column case");
                break;
        }
    }

    return QVariant();
}

int groups::rowCount(const QModelIndex&) const 
{
    return groups_.size();
}

int groups::columnCount(const QModelIndex&) const
{
    return (int)column::last;
}

} // app
