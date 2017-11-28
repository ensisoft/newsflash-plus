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

#define LOGTAG "tasks"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QIcon>
#include <newsflash/warnpop.h>

#include "tasklist.h"
#include "engine.h"
#include "debug.h"
#include "format.h"
#include "types.h"

namespace {

using states  = newsflash::ui::TaskDesc::States;
using errors  = newsflash::ui::TaskDesc::Errors;
using bitflag = newsflash::bitflag<newsflash::ui::TaskDesc::Errors>;

enum class columns { status, done, time, eta, size, desc, sentinel };

const char* str(states s)
{
    switch (s)
    {
        case states::Queued:    return "Queued";
        case states::Waiting:   return "Waiting";
        case states::Active:    return "Active";
        case states::Crunching: return "Crunching";
        case states::Paused:    return "Paused";
        case states::Complete:  return "Complete";
        case states::Error:     return "Error";
    }
    return "???";
}

const char* str(bitflag f)
{
    if (f.test(errors::Dmca))
        return "DMCA";
    else if (f.test(errors::Damaged))
        return "Damaged";
    else if (f.test(errors::Incomplete))
        return "Incomplete";
    else if (f.test(errors::Other))
        return "Other";

    return "Complete";
}

} // namespace

namespace app
{

TaskList::TaskList()
{}

TaskList::~TaskList()
{}

QVariant TaskList::data(const QModelIndex& index, int role) const
{
    const auto col = index.column();
    const auto row = index.row();
    const auto& ui = tasks_[row];

    static const auto infinity = QChar(0x221E);

    if (role == Qt::DisplayRole)
    {
        switch ((columns)col)
        {
            case columns::status:
                if (ui.state == states::Complete)
                {
                    if (ui.completion > 0.0)
                        return str(ui.error);

                    if (ui.error.test(errors::Dmca))
                        return "DMCA";
                    return "Unavailable";
                }
                return str(ui.state);

            case columns::done:
                return QString("%1%").arg(ui.completion, 0, 'f', 2);

            case columns::time:
                return toString(runtime{ui.runtime});

            case columns::eta:
                if (ui.state == states::Waiting ||
                    ui.state == states::Paused ||
                    ui.state == states::Queued)
                    return QString("  %1  ").arg(infinity);
                else if (ui.state == states::Complete)
                    return "";
                else if (ui.runtime < 10 || !(ui.completion > 0))
                    return " hmm...";

                return toString(etatime{ui.etatime});

            case columns::size:
                if (ui.size == 0)
                    return QString("n/a");
                return toString(size{ui.size});

            case columns::desc:
                return fromUtf8(ui.desc);

            case columns::sentinel: break;
        }
    }
    if (role == Qt::DecorationRole)
    {
        if (col != (int)columns::status)
            return QVariant();

        switch (ui.state)
        {
            case states::Queued:
                return QIcon("icons:ico_task_queued.png");
            case states::Waiting:
                return QIcon("icons:ico_task_waiting.png");
            case states::Active:
                return QIcon("icons:ico_task_active.png");
            case states::Crunching:
                return QIcon("icons:ico_task_crunching.png");
            case states::Paused:
                return QIcon("icons:ico_task_paused.png");
            case states::Complete:
                if (ui.error.any_bit())
                    return QIcon("icons:ico_task_damaged.png");

                return QIcon("icons:ico_task_complete.png");

            case states::Error:
                return QIcon("icons:ico_task_error.png");
        }
    }
    return QVariant();
}

QVariant TaskList::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return {};
    if (orientation != Qt::Horizontal)
        return {};

    switch (columns(section))
    {
        case columns::status: return "Status";
        case columns::done:   return "Done";
        case columns::time:   return "Time";
        case columns::eta:    return "ETA";
        case columns::size:   return "Size";
        case columns::desc:   return "Description";
        case columns::sentinel: Q_ASSERT(false);
    }
    return {};
}

int TaskList::rowCount(const QModelIndex&) const
{
    return (int)tasks_.size();
}

int TaskList::columnCount(const QModelIndex&) const
{
    return (int)columns::sentinel;
}

void TaskList::refresh(bool remove_complete)
{
    const auto curSize = tasks_.size();

    g_engine->refreshTaskList(tasks_);

    const auto newSize = tasks_.size();
    if (newSize != curSize)
    {
        QAbstractTableModel::beginResetModel();
        QAbstractTableModel::reset();
        QAbstractTableModel::endResetModel();
    }

    if (remove_complete)
    {
        std::size_t removed = 0;
        for (std::size_t i=0; i<tasks_.size(); ++i)
        {
            const auto& row  = i - removed;
            const auto& task = tasks_[row];
            if (task.state == states::Complete || task.state == states::Error)
            {
                QAbstractTableModel::beginRemoveRows(QModelIndex(), row, row);
                g_engine->killTask(row);
                g_engine->refreshTaskList(tasks_);
                QAbstractTableModel::endRemoveRows();
                ++removed;
            }
        }
    }
    const auto first = QAbstractTableModel::index(0, 0);
    const auto last  = QAbstractTableModel::index(tasks_.size(), (int)columns::sentinel);
    emit dataChanged(first, last);
}

void TaskList::pause(QModelIndexList& list)
{
    manageTasks(list, Action::Pause);
}

void TaskList::resume(QModelIndexList& list)
{
    manageTasks(list, Action::Resume);
}

void TaskList::kill(QModelIndexList& list)
{
    qSort(list);

    int removed = 0;

    for (int i=0; i<list.size(); ++i)
    {
        const auto row = list[i].row() - removed;
        beginRemoveRows(QModelIndex(), row, row);
        g_engine->killTask(row);
        g_engine->refreshTaskList(tasks_);
        endRemoveRows();
        ++removed;
    }
}

void TaskList::moveUp(QModelIndexList& list)
{
    qSort(list.begin(), list.end(), qLess<QModelIndex>());

    manageTasks(list, Action::MoveUp);
}

void TaskList::moveDown(QModelIndexList& list)
{
    qSort(list.begin(), list.end(), qGreater<QModelIndex>());

    manageTasks(list, Action::MoveDown);
}

void TaskList::moveToTop(QModelIndexList& list)
{
    qSort(list.begin(), list.end(), qLess<QModelIndex>());

    const auto distance = list[0].row();

    int min_index = std::numeric_limits<int>::max();
    int max_index = std::numeric_limits<int>::min();
    for (int i=0; i<list.size(); ++i)
    {
        const auto row = list[i].row();
        if (row > max_index)
            max_index = row;
        if (row < min_index)
            min_index = row;

        for (int j=0; j<distance; ++j)
            g_engine->moveTaskUp(row - j);

        list[i] = index(row - distance, 0);
    }

    BOUNDSCHECK(tasks_, max_index);
    BOUNDSCHECK(tasks_, min_index);

    g_engine->refreshTaskList(tasks_);

    auto first = QAbstractTableModel::index(min_index, 0);
    auto last  = QAbstractTableModel::index(max_index, (int)columns::sentinel);
    emit dataChanged(first, last);
}

void TaskList::moveToBottom(QModelIndexList& list)
{
    qSort(list.begin(), list.end(), qGreater<QModelIndex>());

    const auto distance = tasks_.size() - list[0].row() -1;

    int min_index = std::numeric_limits<int>::max();
    int max_index = std::numeric_limits<int>::min();
    for (int i=0; i<list.size(); ++i)
    {
        const auto row = list[i].row();
        if (row > max_index)
            max_index = row;
        if (row < min_index)
            min_index = row;

        for (unsigned j=0; j<distance; ++j)
            g_engine->moveTaskDown(row + j);

        list[i] = index(row + distance, 0);
    }
    BOUNDSCHECK(tasks_, min_index);
    BOUNDSCHECK(tasks_, max_index);

    g_engine->refreshTaskList(tasks_);

    const auto first = QAbstractTableModel::index(min_index, 0);
    const auto last  = QAbstractTableModel::index(max_index, (int)columns::sentinel);
    emit dataChanged(first, last);
}

void TaskList::setGroupSimilar(bool on_off)
{
    QAbstractTableModel::beginResetModel();

    g_engine->setGroupSimilar(on_off);
    g_engine->refreshTaskList(tasks_);

    QAbstractTableModel::reset();
    QAbstractTableModel::endResetModel();
}

void TaskList::clear()
{
    std::size_t removed = 0;
    for (std::size_t i=0; i<tasks_.size(); ++i)
    {
        const auto& item = tasks_[i];
        const auto row   = i - removed;
        if (item.state == states::Complete || item.state == states::Error)
        {
            QAbstractTableModel::beginRemoveRows(QModelIndex(), row, row);
            g_engine->killTask(row);
            QAbstractTableModel::endRemoveRows();
            ++removed;
        }
    }
}

void TaskList::manageTasks(QModelIndexList& list, TaskList::Action a)
{
    int min_index = std::numeric_limits<int>::max();
    int max_index = std::numeric_limits<int>::min();
    for (int i=0; i<list.size(); ++i)
    {
        const auto row = list[i].row();
        if (row > max_index)
            max_index = row;
        if (row < min_index)
            min_index = row;

        switch (a)
        {
            case Action::Pause:
                g_engine->pauseTask(row);
                list[i] = index(row, 0);
                break;

            case Action::Resume:
                g_engine->resumeTask(row);
                list[i] = index(row, 0);
                break;

            case Action::MoveUp:
                g_engine->moveTaskUp(row);
                list[i] = index(row - 1, 0);
                break;

            case Action::MoveDown:
                g_engine->moveTaskDown(row);
                list[i] = index(row + 1, 0);
                break;
        }
    }
    BOUNDSCHECK(tasks_, min_index);
    BOUNDSCHECK(tasks_, max_index);

    g_engine->refreshTaskList(tasks_);

    const auto first = QAbstractTableModel::index(min_index, 0);
    const auto last  = QAbstractTableModel::index(max_index, (int)columns::sentinel);
    emit dataChanged(first, last);
}

} // app
