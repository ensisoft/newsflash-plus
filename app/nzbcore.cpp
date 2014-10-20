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

#define LOGTAG "nzb"

#include <newsflash/config.h>

#include "nzbcore.h"
#include "debug.h"
#include "eventlog.h"
#include "format.h"

namespace app
{

nzbcore::nzbcore()
{
    QObject::connect(&watch_timer_, SIGNAL(timeout()),
        this, SLOT(perform_scan()));

    ppa_ = post_processing_action::mark_processed;

    DEBUG("nzbcore created");
}

nzbcore::~nzbcore()
{
    DEBUG("nzbcore deleted");
}

void nzbcore::watch(bool on_off)
{
    if (!on_off)
        watch_timer_.stop();
    else watch_timer_.start();

    DEBUG(str("Watch timer ", on_off));

    watch_timer_.setInterval(10 * 1000);
}

void nzbcore::perform_scan()
{
    for (int i=0; i<watched_folders_.size(); ++i)
    {

    }
}




} //app