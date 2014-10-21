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
#  include <QString>
#include <newsflash/warnpop.h>

namespace app {
    class settings;
}

class QMenu;

namespace gui
{
    class settings;

    // mainmodule extends the application functionality
    // in a headless fashion, i.e. without providing any user visible GUI.
    // this is in contrast to the mainwidget which provides GUI and functionality.
    class mainmodule
    {
    public:
        struct info {
            QString helpurl;
        };

        virtual ~mainmodule() = default;

        // add the module specific actions to a menu inside the host application
        virtual bool add_actions(QMenu& menu) { return false;}

        // load the module state
        virtual void loadstate(app::settings& s) {};

        // save the module state
        virtual bool savestate(app::settings& s) { return true; };

        // prepare the module for shutdown.
        virtual void shutdown() {};

        // perform first launch activities.
        virtual void first_launch() {}

        virtual info information() const { return {""}; }

        // get a settings widget if any. 
        virtual settings* get_settings(app::settings& s) { return nullptr; }

        // notify that the application settings have been changed.
        virtual void apply_settings(settings* gui, app::settings& backend) {}

        virtual void free_settings(settings* s) {}

        virtual void drop_file(const QString& name) {}
    protected:
    private:
    };
} // gui
