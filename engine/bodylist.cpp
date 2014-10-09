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

#include "bodylist.h"
#include "session.h"

namespace newsflash
{

bodylist::bodylist(std::size_t task, std::size_t account, 
    std::vector<std::string> groups,
    std::vector<std::string> messages) : task_(task), groups_(std::move(groups)), messages_(std::move(messages)), account_(account)
{
    message_ = 0;
    configure_fail_bit_ = false;
}

bodylist::~bodylist()
{}

bool bodylist::submit_configure_command(std::size_t i, session& ses) 
{
    if (i == groups_.size())
        return false;

    // try the ith group in the list of newsgroups that are supposed
    // to carry the articles. we stop after being able to succesfully
    // select whatever group comes first.
    ses.change_group(groups_[i]);
    return true;
}

bool bodylist::receive_configure_buffer(std::size_t i, buffer&& buff)
{
    // if the group exists, its good enough for us here
    if (buff.content_status() == buffer::status::success)
        return true;

    if (i == groups_.size() - 1)
        configure_fail_bit_ = true;

    return false;
}

void bodylist::submit_data_commands(session& ses)
{
    // queue all the article commands that have not been received yet.
    for (auto i=message_; i<messages_.size(); ++i)
        ses.retrieve_article(messages_[i]);
}

void bodylist::receive_data_buffer(buffer&& buff)
{
    // got a response buffer, store it for later
    buffers_.push_back(std::move(buff));

    message_++;
}

} // newsflash