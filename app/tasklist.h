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

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QAbstractTableModel>
#include "newsflash/warnpop.h"

#include <deque>

#include "engine/ui/task.h"

namespace app
{
    class TaskList : public QAbstractTableModel
    {
    public:
        TaskList();
       ~TaskList();

        // QAbstractTableModel
        virtual QVariant data(const QModelIndex& index, int role) const override;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        virtual int rowCount(const QModelIndex&) const override;
        virtual int columnCount(const QModelIndex&) const override;

        void refresh(bool remove_complete);

        void pause(QModelIndexList& list);

        void resume(QModelIndexList& list);

        void kill(QModelIndexList& list);

        void moveUp(QModelIndexList& list);

        void moveDown(QModelIndexList& list);

        void moveToTop(QModelIndexList& list);

        void moveToBottom(QModelIndexList& list);

        void setGroupSimilar(bool on_off);

        void clear();

        const
        newsflash::ui::TaskDesc& getItem(std::size_t i) const
        { return tasks_[i]; }

        const
        newsflash::ui::TaskDesc& getItem(const QModelIndex& i) const
        { return tasks_[i.row()]; }

    private:
        enum class Action {
            Pause, Resume, MoveUp, MoveDown
        };
        void manageTasks(QModelIndexList& list, Action a);

    private:
        std::deque<newsflash::ui::TaskDesc> tasks_;
    };

} // app
