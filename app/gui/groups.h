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

#include <newsflash/sdk/uicomponent.h>
#include <newsflash/sdk/model.h>
#include <newsflash/warnpush.h>
#include <newsflash/warnpop.h>
#include "ui_groups.h"

namespace gui
{
    class Groups : public sdk::uicomponent
    {
        Q_OBJECT

    public:
        Groups(sdk::model& model);
       ~Groups();

        void add_actions(QMenu& menu);
        void add_actions(QToolBar& bar);

        sdk::uicomponent::info get_info() const;

    private slots:
        void on_actionAdd_triggered();
        void on_actionRemove_triggered();
        void on_actionOpen_triggered();
        void on_actionUpdate_triggered();
        void on_actionUpdateOptions_triggered();
        void on_actionDelete_triggered();
        void on_actionClean_triggered();
        void on_actionInfo_triggered();

    private:
        Ui::Groups ui_;

    private:
        sdk::model& model_;

    };

} // gui
