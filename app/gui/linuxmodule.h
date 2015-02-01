// Copyright (c) 2015 Sami Väisänen, Ensisoft 
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
#  include <QObject>
#  include "ui_linux.h"
#include <newsflash/warnpop.h>        

#include "mainmodule.h"
#include "settings.h"

namespace gui
{
    class linuxsettings : public settings
    {
        Q_OBJECT
    public:
        linuxsettings();
       ~linuxsettings();

        virtual bool validate() const override;

    private slots:
        void on_btnDefaultOpenfileCommand_clicked();
        void on_btnDefaultShutdownCommand_clicked();
    private:
        Ui::Linux ui_;
    private:
        friend class linuxmodule;
    };

    class linuxmodule : public mainmodule
    {
    public:
        linuxmodule();
       ~linuxmodule();

        virtual void loadstate(app::settings& s) override;
        virtual bool savestate(app::settings& s) override;
        virtual gui::settings* get_settings() override;
        virtual void apply_settings(settings* gui) override;
        virtual void free_settings(settings* gui) override;

    private:

    };

} // gui