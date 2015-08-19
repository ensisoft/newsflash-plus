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
#  include <QtGui/QSystemTrayIcon>
#  include <QObject>
#include <newsflash/warnpop.h>
#include <newsflash/engine/bitflag.h>
#include "mainmodule.h"

class QAction;

namespace app {
    struct FileInfo;
    struct FilePackInfo;
    struct Event;
    struct Archive;
}

namespace gui
{
    // System tray notifications.
    class Notify : public QObject, public MainModule
    {
        Q_OBJECT

    public:
        Notify();
       ~Notify();

        virtual bool addActions(QMenu& menu) override;
        virtual void loadState(app::Settings& settings) override;
        virtual void saveState(app::Settings& settings) override;
        virtual SettingsWidget* getSettings() override;
        virtual void applySettings(SettingsWidget* widget) override;

    signals:
        void exit();
        void minimize();
        void restore();

    public slots:
        void fileCompleted(const app::FileInfo& file);
        void packCompleted(const app::FilePackInfo& pack);
        void newEvent(const app::Event& event);
        void repairReady(const app::Archive& arc);
        void unpackReady(const app::Archive& arc);
        void windowRestored();

    private slots:
        void trayActivated(QSystemTrayIcon::ActivationReason);
        void actionRestore();
        void actionMinimize();

    private:
        QSystemTrayIcon m_trayIcon;
        QAction* m_minimizeAction;
        QAction* m_restoreAction;
        QAction* m_exitAction;

        enum class NotifyWhen {
            // notifications are enabled.
            Enable, 

            // notify when file is downloaded.
            OnFile, 

            // notify when a batch of files is downloaded.
            OnFilePack, 

            // notify on error event.
            OnError, 

            // notify on warning event.
            OnWarning, 

            // notify when a repair completes.
            OnRepair, 

            // notify when an unpack completes
            OnUnpack
        };
        using Flags = newsflash::bitflag<NotifyWhen>;
        Flags m_notifications;        
    };

} // gui
