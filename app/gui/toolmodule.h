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
#  include <QObject>
#  include "ui_tools.h"
#include <newsflash/warnpop.h>

#include "mainmodule.h"
#include "settings.h"
#include "../tools.h"

namespace gui
{
    class toolsettings : public settings
    {
        Q_OBJECT

    public:
        toolsettings();
       ~toolsettings();

        virtual bool validate() const override;

        void set_tools(std::vector<app::tools::tool> tools);

    private slots:
        void on_btnAdd_clicked();
        void on_btnDel_clicked();
        void on_btnEdit_clicked();
        void on_listTools_doubleClicked(QModelIndex);

    private:
        Ui::Tools ui_;
    private:
        friend class toolmodule;
        std::vector<app::tools::tool> tools_;
    };

    class toolmodule : public QObject, public mainmodule
    {
        Q_OBJECT

    public:
        toolmodule();
       ~toolmodule();

        virtual gui::settings* get_settings() override;

        virtual void apply_settings(settings* gui) override;
        virtual void free_settings(settings* gui) override;

        virtual info information() const override
        { return {"toolmodule", "",}; }
    private:
    };

} // gui