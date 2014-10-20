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
#  include "ui_downloads.h"
#include <newsflash/warnpop.h>
#include <memory>

#include "mainwidget.h"
#include "../tasklist.h"
#include "../connlist.h"

class QEvent;

namespace gui
{
    class downloads : public mainwidget
    {
        Q_OBJECT

    public:
        downloads();
       ~downloads();

        virtual void add_actions(QMenu& menu) override;
        virtual void add_actions(QToolBar& bar) override;

        //virtual void load(const app::datastore& data) override;
        //virtual void save(app::datastore& data) override;

        virtual info information() const override 
        { return {"downloads.html", true, true}; }

    private:
        bool eventFilter(QObject* obj, QEvent* event);

    private:
        Ui::Downloads ui_;

    private:
        app::tasklist tasks_;
        app::connlist conns_;
        int panels_y_pos_;
    };

} // gui
