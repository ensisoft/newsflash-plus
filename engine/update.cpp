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

#include <fstream>
#include <map>
#include <mutex>
#include "filesys.h"
#include "linebuffer.h"
#include "filemap.h"
#include "update.h"
#include "buffer.h"
#include "action.h"
#include "nntp.h"
#include "utf8.h"
#include "logging.h"
#include "cmdlist.h"

namespace newsflash
{

struct update::state {
    std::string folder;
    std::string group;

    std::mutex m;
    std::multimap<std::size_t, std::unique_ptr<filemap>> files;
};

// parse NNTP overview data
class update::parse : public action
{
public:
    parse(std::shared_ptr<state> s, buffer buff) : buffer_(std::move(buff))
    {}

    virtual void xperform() override
    {
        nntp::linebuffer lines(buffer_.content(), buffer_.content_length());
        auto beg = lines.begin();
        auto end = lines.end();
        for (; beg != end; ++beg)
        {
            const auto& line = *beg;
            const auto& pair = nntp::parse_overview(line.start, line.length);
            if (!pair.first)
                continue;

            const auto& xover = pair.second;
            const auto hash = nntp::hashvalue(xover.subject.start, xover.subject.len);

            // calculate bucket.
            // the hash is a 32bit integer so ranges from 0 to 2^32 - 1
            // we devide this into equal sized buckets where each bucket
            // holds 2^20 (1048576) items. each bucket is then backed up
            // by a memory mapped file. we need a maximum of 4096 files
            // to cover the whole range. 
            const auto bucket = hash / 1048576;

            std::lock_guard<std::mutex> lock(state_->m);

            auto& files = state_->files;
            auto lower = files.lower_bound(bucket);
            auto upper = files.upper_bound(bucket);
        }
    }
private:
    std::shared_ptr<state> state_;
    std::size_t first_;
    std::size_t last_;
private:
    buffer buffer_;
};

update::update(std::string path, std::string group) : current_last_(0), current_first_(0)
{
    state_ = std::make_shared<state>();
    state_->folder = std::move(path);
    state_->group  = std::move(group);

    const auto file = fs::joinpath(path, group + ".nfo");

#if defined(LINUX_OS)
    std::ifstream in(file, std::ios::in);
    using string_t = std::string;
#elif defined(WINDOWS_OS)
    std::wifstream in(utf8::decode(file), std::ios::in);
    using string_t = std::wstring;
#endif
    if (in.is_open())
    {
        string_t first;
        string_t last;
        std::getline(in, first);
        std::getline(in, last);
        local_first_ = std::stoull(first);
        local_last_  = std::stoull(last);
    }
    remote_first_ = 0;
    remote_last_  = 0;
}

update::~update()
{}

std::unique_ptr<cmdlist> update::create_commands() 
{
    std::unique_ptr<cmdlist> ret;

    if (remote_last_ == 0)
    {
        // get group information for retrieving the first and last
        // article numbers on the remote server.
        ret.reset(new cmdlist(cmdlist::groupinfo{state_->group}));
    }
    else if (local_last_ < remote_last_)
    {
        cmdlist::overviews ov;
        ov.group = state_->group;

        // while there are newer messages on the remote host we generate
        // commands to retrieve the newer messages starting at the current
        // newest message on the local machine and progressing towards the oldest
        // on the remote host.
        for (int i=0; i<10; ++i)
        {
            const auto f = local_last_ + 1;
            const auto l = f + 1000;
            local_last_ = l;
            std::string range;
            std::stringstream ss;
            ss << f << "-" << l;
            ss >> range;
            ov.ranges.push_back(std::move(range));
        }
        ret.reset(new cmdlist(ov));
    }
    else if (local_first_ > remote_first_)
    {
        // while there are older messages older on the remote host we generate
        // commands to retrieve the older messges starting at the current oldest
        // message on the local machine and progressing towards the oldest on the 
        // remote server.
    }

    return std::move(ret);
}

void update::complete(cmdlist& cmd, 
    std::vector<std::unique_ptr<action>>& next)
{
    if (cmd.cmdtype() == cmdlist::type::groupinfo)
    {
        const auto& contents = cmd.get_buffers();
        const auto& buffer   = contents.at(0);
        const auto& pair = nntp::parse_group(buffer.content(), buffer.content_length());
        if (!pair.first)
            throw std::runtime_error("parse group information failed");

        const auto& group = pair.second;

        remote_first_ = std::stoull(group.first);
        remote_last_  = std::stoull(group.last);

        //LOG_I(state_->group, " has " 
    }
    else
    {
        auto& contents = cmd.get_buffers();
        auto& ranges   = cmd.get_commands();
        for (std::size_t i=0; i<contents.size(); ++i)
        {
            auto& content = contents[i];
            if (content.content_status() != buffer::status::success)
                continue;

            std::unique_ptr<action> p(new parse(state_, std::move(content)));
            p->set_affinity(action::affinity::any_thread);
            next.push_back(std::move(p));
        }
    }
}

void update::complete(action& a, 
    std::vector<std::unique_ptr<action>>& next)
{
    auto* p = dynamic_cast<parse*>(&a);


}

bool update::has_commands() const 
{
    return (remote_last_ != local_last_) || (remote_first_ != local_first_);
}



} // newsflash
