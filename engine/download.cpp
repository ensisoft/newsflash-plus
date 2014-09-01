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

#include <functional>
#include <algorithm>
#include "linebuffer.h"
#include "download.h"
#include "buffer.h"
#include "filesys.h"
#include "logging.h"
#include "action.h"
#include "cmdlist.h"
#include "session.h"
#include "yenc_single_decoder.h"
#include "yenc_multi_decoder.h"
#include "uuencode_decoder.h"
#include "ui/file.h"
#include "ui/download.h"

namespace {

class text_decoder : public newsflash::decoder 
{
public:
    virtual std::size_t decode(const void* data, std::size_t len) override
    {
        const nntp::linebuffer lines((const char*)data, len);
        const auto& iter = lines.begin();
        const auto& line = *iter;

        on_write(line.start, line.length, 0, false);

        return line.length;
    }
    virtual void finish() override
    {}
private:
};

} // namespace

namespace newsflash
{

class download::bodylist : public cmdlist
{
public:
    bodylist(std::deque<std::string> groups, std::deque<std::string> messages) : groups_(std::move(groups)), messages_(std::move(messages))
    {}

    virtual bool is_done(cmdlist::step step) const override
    {
        
    }
    virtual void submit(cmdlist::step step, session& sess) override
    {
        if (step == cmdlist::step::configure)
        {

        }
        else if (step == cmdlist::step::transfer)
        {
            for (const auto& message : messages_)
                sess.retrieve_article(message);
        }
    }

    virtual void receive(cmdlist::step step, buffer& buff) override
    {
    }
private:
    std::deque<std::string> groups_;
    std::deque<std::string> messages_;
    std::deque<buffer> buffers_;
};

download::download(std::size_t id, std::size_t account, const ui::download& details)
{
    state_.err = task::error::none;
    state_.st  = task::state::queued;
    state_.account = account;
    state_.id      = id;
    state_.desc    = details.desc;
    state_.size    = details.size;
    state_.runtime = 0;
    state_.eta     = -1;
    state_.completion = 0.0f;
    state_.damaged = false;

    LOG_D("Task ", id_, " created");    
}

download::~download()
{
    LOG_D("Task ", id_, " deleted");
}

void download::start()
{
    state_.st = task::state::waiting;


}

} // newsflash

