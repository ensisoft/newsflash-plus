// Copyright (c) 2010-2015 Sami Väisänen, Ensisoft 
//
// http://www.ensisoft.com
// 
// This software is copyrighted software. Unauthorized hacking, cracking, distribution
// and general assing around is prohibited.
// Redistribution and use in source and binary forms, with or without modification,
// without permission are prohibited.
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QIODevice>
#include <newsflash/warnpop.h>
#include "nzbthread.h"
#include "debug.h"
#include "format.h"

namespace app
{

NZBThread::NZBThread(std::unique_ptr<QIODevice> io) : io_(std::move(io)), error_(NZBError::None)
{
    DEBUG("Created nzbhread");
}

NZBThread::~NZBThread()
{
    DEBUG("Destroyed nzbhread");
}

void NZBThread::run() 
{
    std::vector<NZBContent> data;
    const auto result = parseNZB(*io_.get(), data);

    std::lock_guard<std::mutex> lock(mutex_);
    data_  = std::move(data);
    error_ = result;

    emit complete();

    DEBUG("nzbthread done...");
}

NZBError NZBThread::result(std::vector<NZBContent>& data)
{
    std::lock_guard<std::mutex> lock(mutex_);

    data = std::move(data_);
    return error_;
}    

} // app