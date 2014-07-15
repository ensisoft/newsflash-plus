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

#include <newsflash/warnpush.h>
#  include <QIODevice>
#include <newsflash/warnpop.h>

#include "nzbthread.h"
#include "debug.h"
#include "format.h"

namespace app
{

nzbthread::nzbthread(std::unique_ptr<QIODevice> io) : io_(std::move(io)), error_(nzberror::none)
{
    DEBUG("Created nzbhread");
}

nzbthread::~nzbthread()
{
    DEBUG("Destroyed nzbhread");
}

void nzbthread::run() 
{
    DEBUG(str("nzbthread::run _1", QThread::currentThreadId()));

    std::vector<nzbcontent> data;
    const auto result = parse_nzb(*io_.get(), data);

    std::lock_guard<std::mutex> lock(mutex_);
    data_  = std::move(data);
    error_ = result;

    emit complete();

    DEBUG("nzbthread done...");
}

nzberror nzbthread::result(std::vector<nzbcontent>& data)
{
    std::lock_guard<std::mutex> lock(mutex_);

    data = std::move(data_);
    return error_;
}    

} // app