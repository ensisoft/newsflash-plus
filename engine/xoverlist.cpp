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

#include <boost/lexical_cast.hpp>
#include <algorithm>
#include "xoverlist.h"
#include "protocol.h"
#include "buffer.h"

namespace newsflash
{

xoverlist::xoverlist(std::string group) : group_(std::move(group)), start_(0), end_(0), first_(false), error_(false)
{}

bool xoverlist::run(protocol& proto)
{
    // the fist thread to enter the xoverlist will 
    // need to request information about the group.
    // this information is then used to generate
    // the actual xover requests
    if (!first_thread())
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!configured_)
            cond_.wait(lock);
    }
    else
    {
        std::unique_lock<std::mutex> lock(mutex_);        
        protocol::groupinfo info {0};
        if (!proto.group(group_, info))
            error_ = false;
        else
        {
            start_ = info.low_water_mark;
            end_   = info.high_water_mark;
        }
        configured_ = true;
        cond_.notify_all();
    }

    std::unique_lock<std::mutex> lock(mutex_);
    if (error_)
        return false;

    if (start_ == end_)
        return false;

    const std::uint64_t XOVER_BATCH_SIZE = 2000;

    const auto start = start_;
    const auto size  = std::min(end_ - start_, XOVER_BATCH_SIZE);
    const auto end   = start + size;
    start_ += size;

    lock.unlock();

    xoverlist::xover xover;
    xover.start = start;
    xover.end   = end;

    xover.buff = std::make_shared<buffer>(1024 * 1024);
    proto.xover(boost::lexical_cast<std::string>(start),
        boost::lexical_cast<std::string>(end), *xover.buff);

    on_xover(xover);

    return true;
}


bool xoverlist::first_thread()
{
    std::lock_guard<std::mutex> lock(mutex_);
    bool ret = first_;
    first_ = false;
    return ret;
}

} // newsflash
