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
#  include "ui_nzbcore.h"
#include <newsflash/warnpop.h>
#include "mainwidget.h"
#include "mainmodule.h"
#include "settings.h"
#include "../nzbcore.h"

namespace gui
{
    class nzbfile;

    class nzbsettings : public settings
    {
        Q_OBJECT
    public:
        nzbsettings();
       ~nzbsettings();

        virtual bool validate() const override;

    private slots:
        void on_btnAddWatchFolder_clicked();
        void on_btnDelWatchFolder_clicked();
        void on_btnSelectDumpFolder_clicked();
        void on_btnSelectDownloadFolder_clicked();
    private:
        Ui::NZBCore ui_;
    private:
        friend class nzbcore;
    };

    // nzbcore UI functionality
    class nzbcore : public QObject, public mainmodule
    {
        Q_OBJECT
    public:
        nzbcore();
       ~nzbcore();

        virtual bool add_actions(QMenu& menu) override;
        virtual void loadstate(app::settings& s) override;
        virtual bool savestate(app::settings& s) override;
        virtual info information() const override
        { return {"nzbcore", "nzb.html"}; }

        virtual settings* get_settings() override;
        virtual void apply_settings(settings* gui) override;
        virtual void free_settings(settings* s) override;

        virtual void drop_file(const QString& file) override;

    private slots:
        void downloadTriggered();
        void displayTriggered();

    private:
        app::nzbcore module_;
    };
} // gui