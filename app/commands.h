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
#  include <QtGui/QIcon>
#  include <QAbstractTableModel>
#  include <QByteArray>
#  include <QObject>
#  include <QProcess>
#include <newsflash/warnpop.h>
#include <newsflash/engine/bitflag.h>
#include <memory>
#include <vector>
#include "engine.h"

namespace app
{
    class Settings;
    struct FileInfo;
    struct FilePackInfo;
    struct Archive;

    class Commands : public QObject
    {
        Q_OBJECT

    public:
        class Condition 
        {
        public:
            Condition(const QString& lhs, const QString& op, const QString& rhs) :
                lhs_(lhs), op_(op), rhs_(rhs)
            {}
            Condition() 
            {}

            virtual bool evaluate(const app::FileInfo& info) const;
            virtual bool evaluate(const app::FilePackInfo& pack) const;
            virtual bool evaluate(const app::Archive& arc) const;

            QString getLHS() const 
            { return lhs_; }

            QString getRHS() const 
            { return rhs_; }

            QString getOp() const 
            { return op_; }

        private:
            QString lhs_;
            QString op_;
            QString rhs_;
        };

        class Command
        {
        public:
            enum class When {
                OnFileDownload,
                OnPackDownload,
                OnArchiveRepair,
                OnArchiveUnpack
            };

            using WhenFlags = newsflash::bitflag<When>;

            Command(QString exec, QString args);
            Command(QString exec, QString args, QString comment);
            Command(QString exec, QString args, QString comment, Condition cond);


            Command();

            QString exec() const 
            { return exec_; }

            QString args() const 
            { return args_; }

            QString comment() const 
            { return comment_; }

            QIcon icon() const;

            WhenFlags when() const 
            { return when_; }

            void setComment(const QString& comment)
            { comment_ = comment; }

            void setEnableCommand(bool onOff)
            { enableCommand_ = onOff; }

            void setEnableCondition(bool onOff)
            { enableCond_ = onOff; }

            void setExec(const QString& exec)
            { exec_ = exec; }

            void setArgs(const QString& args)
            { args_ = args; }

            void setWhen(WhenFlags w)
            { when_ = w; }

            bool isEnabled() const
            { return enableCommand_; }

            bool isCondEnabled() const 
            { return enableCond_; }

            quint32 guid() const
            { return guid_; }

            void onFile(const app::FileInfo& info);
            void onFilePack(const app::FilePackInfo& info);
            void onUnpack(const app::Archive& arc);
            void onRepair(const app::Archive& arc);

            std::vector<Condition> getConditionsCopy() const 
            { return conds_; }

            void setConditionsCopy(std::vector<Condition> cond)
            { conds_ = std::move(cond); }

            const
            std::vector<Condition>& getConditions() const 
            { return conds_; }

            void appendCondition(Condition c) 
            { conds_.push_back(std::move(c)); }

            void removeCondition(std::size_t i)
            { 
                auto it = std::begin(conds_);
                it += i;
                conds_.erase(it);
            }

        private:
            quint32 guid_;            
            QString exec_;
            QString args_;
            QString comment_;
            mutable QIcon icon_;
        private:
            bool enableCommand_;
            bool enableCond_;
        private:
            WhenFlags when_;
        private:
            std::vector<Condition> conds_;
        };

        Commands();
       ~Commands();

        void loadState(Settings& settings);
        void saveState(Settings& settings);

        void firstLaunch();

        using CmdList = std::vector<Command>;

        CmdList getCommandsCopy() const
        { return commands_; }

        void setCommandsCopy(CmdList commands)
        { commands_ = commands; }

    public slots:
        void fileCompleted(const app::FileInfo& info);
        void packCompleted(const app::FilePackInfo& info);
        void repairFinished(const app::Archive& arc);
        void unpackFinished(const app::Archive& arc);

    private:
        CmdList commands_;
    };

} // app