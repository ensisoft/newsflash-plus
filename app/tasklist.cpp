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

using states  = newsflash::ui::task::states;
using flags   = newsflash::ui::task::errors;
using bitflag = newsflash::bitflag<newsflash::ui::task::errors>;

enum class columns { status, done, time, eta, size, desc, sentinel };    

const char* str(states s)
{
    switch (s)
    {
        case states::queued: return "Queued";
        case states::waiting: return "Waiting";
        case states::active: return "Active";
        case states::paused: return "Paused";
        case states::complete: return "Complete";
        case states::error: return "Error";
    }
    return "???";
}

const char* str(bitflag f)
{
    if (f.test(flags::dmca))
        return "DMCA";
    else if (f.test(flags::damaged))
        return "Damaged";    
    else if (f.test(flags::unavailable))
        return "Incomplete";

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
                if (ui.state == states::complete)
                    return str(ui.error);
                return str(ui.state);

            case columns::done:
                return QString("%1%").arg(ui.completion, 0, 'f', 2);

            case columns::time:
                return toString(runtime{ui.runtime});

            case columns::eta:
                if (ui.state == states::waiting || 
                    ui.state == states::paused ||
                    ui.state == states::queued)
                    return QString("  %1  ").arg(infinity);
                else if (ui.state == states::complete)
                    return "";
                else if (ui.runtime < 10)
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
            case states::queued: 
                return QIcon("icons:ico_task_queued.png");
            case states::waiting: 
                return QIcon("icons:ico_task_waiting.png");
            case states::active: 
                return QIcon("icons:ico_task_active.png");
            case states::paused: 
                return QIcon("icons:ico_task_paused.png");   
            case states::complete: 
                if (ui.error.any_bit())
                    return QIcon("icons:ico_task_damaged.png");

                return QIcon("icons:ico_task_complete.png");                

            case states::error: 
                return QIcon("icons:ico_task_error.png");                
        }
    }
    return QVariant();
}

QVariant TaskList::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

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
    return QVariant();
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
    const auto cur_size = tasks_.size();

    g_engine->refreshTaskList(tasks_);

    if (remove_complete)
    {
        std::size_t removed = 0;
        for (std::size_t i=0; i<tasks_.size(); ++i)
        {
            const auto& row  = i - removed;            
            const auto& task = tasks_[row];
            if (task.state == states::complete || task.state == states::error)
            {
                beginRemoveRows(QModelIndex(), row, row);
                g_engine->killTask(row);
                g_engine->refreshTaskList(tasks_);
                endRemoveRows();
                ++removed;
            }
        }
    }

    if (tasks_.size() != cur_size)
    {
        reset();        
    }
    else
    {
        auto first = QAbstractTableModel::index(0, 0);
        auto last  = QAbstractTableModel::index(tasks_.size(), (int)columns::sentinel);
        emit dataChanged(first, last);
    }
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
    Q_ASSERT(min_index >= 0);
    Q_ASSERT(max_index < tasks_.size());

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
    Q_ASSERT(min_index >= 0);
    Q_ASSERT(max_index < tasks_.size());

    g_engine->refreshTaskList(tasks_);

    auto first = QAbstractTableModel::index(min_index, 0);
    auto last  = QAbstractTableModel::index(max_index, (int)columns::sentinel);
    emit dataChanged(first, last);
}

void TaskList::setGroupSimilar(bool on_off)
{
    g_engine->setGroupSimilar(on_off);
    g_engine->refreshTaskList(tasks_);
    reset();
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
    Q_ASSERT(min_index >= 0);
    Q_ASSERT(max_index < tasks_.size());    

    g_engine->refreshTaskList(tasks_);

    auto first = QAbstractTableModel::index(min_index, 0);
    auto last  = QAbstractTableModel::index(max_index, (int)columns::sentinel);
    emit dataChanged(first, last);    
}

} // app
