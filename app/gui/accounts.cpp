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

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QtGui/QToolBar>
#  include <QtGui/QMenu>
#  include <QAbstractListModel>
#include <newsflash/warnpop.h>

#include "accounts.h"

namespace {

    class model : public QAbstractListModel
    {
    public:
        enum class type {
            server, group
        };
        struct item {
            model::type type;
            QString text;
            int parent;
            int index;
        };

        model(QObject* parent) : QAbstractListModel(parent)
        {}

       ~model()
        {}
    private:
    };

} // namespace

namespace gui
{

Accounts::Accounts(app::accounts& accounts) : accounts_(accounts)
{
    ui_.setupUi(this);
}

Accounts::~Accounts()
{}

void Accounts::add_actions(QMenu& menu)
{
    menu.addAction(ui_.actionServerAdd);
    menu.addAction(ui_.actionServerDelete);
    menu.addSeparator();
    menu.addAction(ui_.actionServerSortGroups);
    menu.addAction(ui_.actionServerDownloadList);
    menu.addAction(ui_.actionGroupAdd);
    menu.addAction(ui_.actionGroupAddByName);
    menu.addSeparator();
    menu.addAction(ui_.actionGroupUpdate);
    menu.addAction(ui_.actionGroupUpdateWO);
    menu.addSeparator();
    menu.addAction(ui_.actionGroupOpen);
    menu.addSeparator();
    menu.addAction(ui_.actionGroupDelete);
    menu.addSeparator();
    menu.addAction(ui_.actionGroupEraseContent);
    menu.addSeparator();
    menu.addAction(ui_.actionGroupPurgeContent);

}

void Accounts::add_actions(QToolBar& bar)
{
    bar.addAction(ui_.actionServerAdd);
    bar.addAction(ui_.actionServerDelete);
    bar.addSeparator();
    bar.addAction(ui_.actionServerSortGroups);
    bar.addAction(ui_.actionGroupAdd);
    bar.addSeparator();
    bar.addAction(ui_.actionGroupUpdate);
    bar.addAction(ui_.actionGroupOpen);
    bar.addSeparator();
    bar.addAction(ui_.actionGroupDelete);
    bar.addAction(ui_.actionGroupEraseContent);
    bar.addAction(ui_.actionGroupPurgeContent);
}


// void Accounts::activate(QWidget*)
// {}

// void Accounts::deactivate()
// {}

sdk::uicomponent::info Accounts::get_info() const
{
    const static sdk::uicomponent::info info {
        "accounts.html", true
    };
    return info;
}

} // gui
