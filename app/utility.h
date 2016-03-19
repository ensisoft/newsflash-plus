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
#  include <QtAlgorithms>
#  include <QModelIndex>
#  include <QString>
#  include <QStringList>
#include <newsflash/warnpop.h>
#include <algorithm>
#include <string>
#include <vector>

class QAbstractTableModel;
class QTableView;
class QCheckBox;
class QLineEdit;
class QComboBox;
class QPixmap;
class QIcon;
class QAction;

namespace app
{

QString suggestName(const std::vector<std::string>& subjectLines);

QString joinPath(const QString& lhs, const QString& rhs);

QPixmap toGrayScale(const QPixmap& p);

QPixmap toGrayScale(const QString& pixmap);

QString cleanPath(const QString& str);

// calculate the sum of the sizes of the files in the given folder.
// if there are no files or the folder doesn't exist the sum is 0.
// the operation is not recursive and only considers real files
// i.e. symlinks are not counted for.
quint64 sumFileSizes(const QString& folder);

bool removeDirectory(const QString& folder);

class Settings;

inline
void sortAscending(QModelIndexList& list)
{
    qSort(list.begin(), list.end(), qLess<QModelIndex>());
}

inline
void sortDescending(QModelIndexList& list)
{
    qSort(list.begin(), list.end(), qGreater<QModelIndex>());
}

void saveTableLayout(const QString& key, const QTableView* view, Settings& settings);
void loadTableLayout(const QString& key, QTableView* view, const Settings& settings);

void loadState(const QString& key, QCheckBox* chk, Settings& settings);
void loadState(const QString& key, QLineEdit* edt, Settings& settings);
void loadState(const QString& key, QComboBox* cmb, Settings& settings);
void loadState(const QString& key, QAction* act, Settings& settings);
void saveState(const QString& key, const QCheckBox* chk, Settings& settings);
void saveState(const QString& key, const QLineEdit* edt, Settings& settings);
void saveState(const QString& key, const QComboBox* cmb, Settings& settings);
void saveState(const QString& key, const QAction* act, Settings& settings);

template<typename Class, typename Member>
struct CompareLess {
    typedef Member (Class::*MemPtr);

    CompareLess(MemPtr p) : ptr_(p)
    {}

    bool operator()(const Class& lhs, const Class& rhs) const
    {
        return lhs.*ptr_ < rhs.*ptr_;
    }
private:
    MemPtr ptr_;
};

template<typename Class, typename Member>
struct CompareGreater {
    typedef Member (Class::*MemPtr);

    CompareGreater(MemPtr p) : ptr_(p)
    {}

    bool operator()(const Class& lhs, const Class& rhs) const
    {
        return lhs.*ptr_ > rhs.*ptr_;
    }
private:
    MemPtr ptr_;
};

template<typename Class, typename Member>
CompareLess<Class, Member> less(Member Class::*p)
{
    return CompareLess<Class, Member>(p);
}

template<typename Class, typename Member>
CompareGreater<Class, Member> greater(Member Class::*p)
{
    return CompareGreater<Class, Member>(p);
}

template<typename Container, typename MemPtr>
void sort(Container& container, Qt::SortOrder order, MemPtr p)
{
    if (order == Qt::AscendingOrder)
        std::sort(std::begin(container), std::end(container), less(p));
    else std::sort(std::begin(container), std::end(container), greater(p));
}

template<typename Iterator, typename MemPtr>
void sort(Iterator beg, Iterator end, Qt::SortOrder order, MemPtr p)
{
    if (order == Qt::AscendingOrder)
        std::sort(beg, end, less(p));
    else std::sort(beg, end, greater(p));
}

template<typename InputIt, typename OutputIt, typename MemPtr>
void merge(InputIt first1, InputIt last1,
    InputIt first2, InputIt last2, OutputIt out, Qt::SortOrder order, MemPtr p)
{
    if (order == Qt::AscendingOrder)
        std::merge(first1, last1, first2, last2, out, less(p));
    else std::merge(first1, last1, first2, last2, out, greater(p));
}

inline
quint64 MB(int mb)
{
    return mb * 1024 * 1024;
}

inline
quint64 KB(int kb)
{
    return kb * 1024;
}

} // app