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
#  include <QtGui/QWidget>
#  include <QString>
#  include <QObject>
#include "newsflash/warnpop.h"

class QMenu;
class QToolBar;

namespace app {
    class Settings;
} // app

namespace gui
{
    class SettingsWidget;
    class Finder;

    // mainwidget objects sit in the mainwindow's main tab 
    // and provides GUI and features to the application.
    // the different between mainwidget and mainmodule is that
    // mainmodules are simpler headless versions that no not provide
    // a user visible GUI
    class MainWidget : public QWidget
    {
        Q_OBJECT

    public:
        struct info {
            // this is the URL to the help (file). If it specifies a filename
            // it is considered to be a help file in the application's help installation
            // folder. Otherwise an absolute path or URL can be specified.            
            QString helpurl;

            // Whether the component should be visible by default (on first launch).
            bool initiallyVisible;
        };

        virtual ~MainWidget() = default;
        
        // Add the component specific menu actions to a menu inside the host application
        virtual void addActions(QMenu& menu) {}
        
        // Add the component specific toolbar actions to a toolbar in the host application
        virtual void addActions(QToolBar& bar) {}

        // This function is invoked when this ui component is getting activated (becomes visible)
        // in the host GUI.
        virtual void activate(QWidget* parent) {}
        
        // This function is invoked when this ui component is hidden in the host GUI.
        virtual void deactivate() {}

        // load the widget/component state on application startup
        virtual void loadState(app::Settings& s) {}

        // save the widget/component state.
        // can throw an exception.
        virtual void saveState(app::Settings& s) {};

        // prepare the widget/component for shutdown
        virtual void shutdown() {}

        // startup up the widget after application has loaded
        // all the settings & data.
        virtual void startup() {}

        // refresh the widget contents
        virtual void refresh(bool isActive) {}

        // perform first launch activities.
        virtual void firstLaunch() {}

        // get information about the widget/component.
        virtual info getInformation() const { return {"", false}; }

        // get a settings widget if any. the object ownership is 
        // transferred to the caller.
        virtual SettingsWidget* getSettings() { return nullptr; }

        virtual void applySettings(SettingsWidget* gui) {}

        // notify that application settings have changed. 
        virtual void freeSettings(SettingsWidget* s) {}

        virtual Finder* getFinder() { return nullptr; }

        virtual void updateRegistration(bool success) {};

    signals:
        // request the widget to have it's toolbar updated.
        void updateMenu(MainWidget* self);

        // request the settings to be displayed.
        void showSettings(MainWidget* self);

    private:
    };

} // gui
