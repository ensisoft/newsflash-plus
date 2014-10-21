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

#pragma once

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QtGui/QWidget>
#  include <QString>
#include <newsflash/warnpop.h>

class QMenu;
class QToolBar;

namespace app {
    class settings;
} // app

namespace gui
{
    class settings;

    // mainwidget objects sit in the mainwindow's main tab 
    // and provides GUI and features to the application.
    // the different between mainwidget and mainmodule is that
    // mainmodules are simpler headless versions that no not provide
    // a user visible GUI
    class mainwidget : public QWidget
    {
    public:
        struct info {
            // this is the URL to the help (file). If it specifies a filename
            // it is considered to be a help file in the application's help installation
            // folder. Otherwise an absolute path or URL can be specified.            
            QString helpurl;

            // Whether the component should be visible by default (on first launch).
            bool visible_by_default;            

            // permant widgets are only hidden when closed.
            // non-permanent widgets are deleted when closed.
            bool permanent;
        };

        virtual ~mainwidget() = default;
        
        // Add the component specific menu actions to a menu inside the host application
        virtual void add_actions(QMenu& menu) {}
        
        // Add the component specific toolbar actions to a toolbar in the host application
        virtual void add_actions(QToolBar& bar) {}

        // This function is invoked when this ui component is getting activated (becomes visible)
        // in the host GUI.
        virtual void activate(QWidget* parent) {}
        
        // This function is invoked when this ui component is hidden in the host GUI.
        virtual void deactivate() {}

        // this function is called when the widget is closed 
        virtual void close_widget() {}

        // load the widget/component state on application startup
        virtual void loadstate(app::settings& s) {}

        // save the widget/component state
        virtual bool savestate(app::settings& s) { return true; }

        // prepare the widget/component for shutdown
        virtual void shutdown() {}

        // refresh the widget contents
        virtual void refresh() {}

        // perform first launch activities.
        virtual void first_launch(bool add_account) {}

        // get information about the widget/component.
        virtual info information() const { return {"", false, false}; }

        // get a settings widget if any. the object ownership is 
        // transferred to the caller.
        virtual settings* get_settings(app::settings& s) { return nullptr; }

        virtual void apply_settings(settings* gui, app::settings& backed) {}

        // notify that application settings have changed. 
        virtual void free_settings(settings* s) {}

        virtual void drop_file(const QString& file) {};

    private:
    };

} // gui
