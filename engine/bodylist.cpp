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

#include "bodylist.h"
#include "protocol.h"
#include "buffer.h"

namespace engine
{
bodylist::bodylist(std::deque<std::string> groups,
                  const std::deque<std::string>& articles)
    : groups_(std::move(groups))

{
    for (std::size_t i=0; i<articles.size(); ++i)
        articles_.push_back(article { articles[i], i});
}


bodylist::~bodylist()
{}

bool bodylist::run(protocol& proto)
{
    article next;
    if (!dequeue(next))
        return false;

    bodylist::body body;

    try
    {
        body.id      = next.id;
        body.article = next.messageid;
        body.status  = bodylist::status::unavailable;
        body.buff.reserve(1024 * 1024);

        for (const auto& group : groups_)
        {
            if (group.empty())
                continue;

            if (!proto.group(group))
                continue;

            const auto ret = proto.body(next.messageid, body.buff);
            if (ret == protocol::status::success)
                body.status = status::success;
            else if (ret == protocol::status::dmca)
                body.status = status::dmca;
            else if (ret == protocol::status::unavailable)
                body.status = status::unavailable;

            if (body.status != status::unavailable)
                break;
        }
    }
    catch (const std::exception&)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        articles_.push_front(next);
        throw ;
    }

    on_body(std::move(body));

    return true;
}

bool bodylist::dequeue(article& next)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (articles_.empty())
        return false;

    next = articles_.front();
    articles_.pop_front();
    return true;
}

} // engine
