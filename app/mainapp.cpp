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

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <boost/version.hpp>
#  include <QStringList>
#  include <QFile>
#  include <QDir>
#  include <QTimer>
#include <newsflash/warnpop.h>

#include <newsflash/engine/engine.h>
#include <newsflash/engine/account.h>
#include <newsflash/engine/listener.h>

#include <vector>

#include "mainapp.h"
#include "eventlog.h"
#include "valuestore.h"
#include "accounts.h"
#include "groups.h"
#include "debug.h"

namespace app
{

mainapp::mainapp()
{
    DEBUG("Application created");
}

mainapp::~mainapp()
{
    DEBUG("Application deleted");
}

QAbstractItemModel* mainapp::get_accounts_data()
{
    return &accounts_;
}

QAbstractItemModel* mainapp::get_event_data()
{
    auto& log = eventlog::get();
    return &log;
}

QAbstractItemModel* mainapp::get_groups_data()
{
    return &groups_;
}

void mainapp::handle(const newsflash::error& error)
{}


void mainapp::acknowledge(const newsflash::file& file)
{}


void mainapp::notify()
{}

} // app