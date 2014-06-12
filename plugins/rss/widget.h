// Copyright (c) 2014 Sami Väisänen, Ensisoft 
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

#include <newsflash/sdk/widget.h>
#include <newsflash/sdk/rssmodel.h>
#include <newsflash/sdk/window.h>
#include <newsflash/sdk/datastore.h>

#include <newsflash/warnpush.h>

#include <newsflash/warnpop.h>
#include <memory>
#include "ui_rss.h"

namespace rss
{
    class widget : public sdk::widget
    {
        Q_OBJECT

    public:
        widget(sdk::window& win);
       ~widget();

        virtual void add_actions(QMenu& menu) override;
        virtual void add_actions(QToolBar& bar) override;

        virtual void activate(QWidget*) override;

        virtual void save(sdk::datastore& store) override;
        virtual void load(const sdk::datastore& store) override;

        virtual info information() const override;

    private:
        Ui::RSS ui_;

    private:
        sdk::window& win_;
        sdk::rssmodel* rss_;
    };

} // rss
