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
#include <memory>
#include "engine.h"

namespace app
{
    class Settings;

    class Commands
    {
    public:
        class Command
        {
        public:
            Command(const QString& name, const QString& exec, const QString& file, const QString& args);

            Command();

            QString name() const 
            { return name_; }

            QString exec() const 
            { return exec_; }

            QString file() const 
            { return file_; }

            QString args() const 
            { return args_; }

            QString comment() const 
            { return comment_; }

            QIcon icon() const;

            void setComment(const QString& comment)
            { comment_ = comment; }

            void setEnabled(bool onOff)
            { enabled_ = onOff; }

            void setExec(const QString& exec)
            { exec_ = exec; }

            void setName(const QString& name)
            { name_ = name; }

            void setArgs(const QString& args)
            { args_ = args; }

            void setFile(const QString& file)
            { file_ = file; }

            bool isEnabled() const
            { return enabled_; }

            quint32 guid() const
            { return guid_; }

            void startNewInstance(const app::DataFileInfo& info);
            void startNewInstance(const app::FileBatchInfo& info);

        private:
            QString name_;
            QString exec_;
            QString file_;
            QString args_;
            mutable QString comment_;
            mutable QIcon icon_;
        private:
            quint32 guid_;
        private:
            bool enabled_;
        };

        Commands();
       ~Commands();

        void loadState(Settings& settings);
        void saveState(Settings& settings);

        using CmdList = std::vector<Command>;

        CmdList getCommandsCopy() const
        { return commands_; }

        void setCommandsCopy(CmdList commands)
        { commands_ = commands; }

    private: 
        void fileCompleted(const app::DataFileInfo& info);
        void batchCompleted(const app::FileBatchInfo& info);

    private:
        CmdList commands_;
    };

} // app