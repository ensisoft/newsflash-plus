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

#include <newsflash/sdk/newsflash.h>
#include <newsflash/engine/listener.h>
#include <QObject>
#include <vector>
#include <memory>
#include "action.h"

namespace gui {
    class MainWindow;
}

namespace newsflash {
    class engine;
}

namespace sdk {
    class uicomponent;
}

namespace app
{
    class valuestore;
    class accounts;
    class eventlog;
    class action;

    // we need a class so that we can connect Qt signals
    class mainapp : public QObject, public newsflash::listener
    {
        Q_OBJECT

    public:
        mainapp();
       ~mainapp();
       
        int run(int argc, char* argv[]);

        bool shutdown();

        // open a generic resource using system's
        // "open" command. typically such a command
        // inspects whatever tool/file association
        // is available in the system and then selects an
        // appropriate application for opening the resource.
        bool open(const QString& resouce);

        // open a help page.
        bool open_help(const QString& page);

        uid_t submit(std::unique_ptr<action> action);

        // listener api
        virtual void handle(const newsflash::error& error) override;
        virtual void acknowledge(const newsflash::file& file) override;
        virtual void notify() override;

    private slots:
        void welcome_new_user();
        void modify_account(std::size_t i);

    private:


        bool load_valuestore();
        bool save_valuestore();

        void attach_views();
        void detach_views();

    private:
        gui::MainWindow* gui_window_;        

    private:
        app::valuestore* valuestore_;
        app::eventlog* eventlog_;
        app::accounts* accounts_;

    private:
        std::vector<sdk::uicomponent*> views_;
        std::vector<std::unique_ptr<action> > pending_;

    private:
        newsflash::engine* engine_;
        bool virgin_;        
    };

} // app
