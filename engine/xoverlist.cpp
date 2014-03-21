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

namespace engine
{

xoverlist::xoverlist(std::string group) : group_(std::move(group)), first_(true), error_(false), configured_(false)
{}

bool xoverlist::run(protocol& proto)
{
    // the fist thread to enter the xoverlist will 
    // need to request information about the group.
    // this information is then used to generate
    // the actual xover requests

    // todo: there's a potential issue here that if an exception
    // occurs during the first call to the protocol::group then 
    // the condition will never get signaled and all the threads 
    // will get blocked there eventually. fix this!
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
        {
            error_ = true;
            if (on_unavailable)
                on_unavailable();
        }
        else
        {
            const std::uint64_t XOVER_BATCH_SIZE = 2000;

            if (info.article_count && info.high_water_mark >= info.low_water_mark)
            {
                // generate the range objects
                // the high and low water mark are inclusive article numbers.                
                const auto lo = info.low_water_mark;
                const auto hi = info.high_water_mark;
                const auto total = hi - lo + 1;
                const auto max   = (total + (XOVER_BATCH_SIZE - 1)) / XOVER_BATCH_SIZE;
                for (size_t i=0; i<max; ++i)
                {
                    const auto beg = lo + i * XOVER_BATCH_SIZE;
                    const auto rem = hi - beg;
                    const auto end = beg + std::min(XOVER_BATCH_SIZE-1, rem);
                    const auto next = range {beg, end};
                    ranges_.push_back(next);
                }
            }

            if (on_prepare_ranges)
                on_prepare_ranges(ranges_.size());            
        }
        configured_ = true;
        cond_.notify_all();
    }

    range next;
    if (!dequeue(next))
        return false;

    // if we're this far then it we've got the initial group information
    // already from the server and the group should be availble.
    // however because it's possible that the connection goes down
    // and then cmdlist is restarted the group must be selected again.
    // however this is not expected to fail now.
    if (!proto.group(group_))
        throw std::runtime_error("unexpected failure changing group");    

    xoverlist::xover xover;

    try
    {
        xover.start = next.start;
        xover.end   = next.end;
        xover.buff.reserve(1024 * 1024);

        proto.xover(boost::lexical_cast<std::string>(next.start),
            boost::lexical_cast<std::string>(next.end), xover.buff);
    }
    catch (const std::exception& e)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ranges_.push_front(next);
        throw;
    }

    on_xover(std::move(xover));

    return true;
}


bool xoverlist::first_thread()
{
    std::lock_guard<std::mutex> lock(mutex_);
    bool ret = first_;
    first_ = false;
    return ret;
}

bool xoverlist::dequeue(range& next)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (error_)
        return false;

    if (ranges_.empty())
        return false;

    next = ranges_.front();
    ranges_.pop_front();
    return true;
}

} // engine
