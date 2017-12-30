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

#include "newsflash/config.h"

#include <string>
#include <fstream>
#include <mutex>

#include "nntp.h"
#include "linebuffer.h"
#include "listing.h"
#include "buffer.h"
#include "cmdlist.h"
#include "assert.h"

namespace newsflash
{

struct Listing::State {
    std::mutex mutex;
    std::size_t last_parse_offset = 0;
    bool discard = false;
    bool commands = true;
    bool complete = false;

    Listing::OnProgress progress_callback;
    std::vector<NewsGroup> groups;
};

Listing::Listing()
{
    state_ = std::make_shared<State>();
}

Listing::~Listing()
{
    std::lock_guard<std::mutex> lock(state_->mutex);
    state_->discard = true;
}

std::shared_ptr<CmdList> Listing::CreateCommands()
{
    std::shared_ptr<CmdList> cmd(new CmdList(CmdList::Listing{}));
    if (state_->progress_callback)
    {
        cmd->SetIntermediateContentBufferCallback(std::bind(ParseIntermediateBuffer,
            std::placeholders::_1, std::placeholders::_2, state_));
    }
    state_->commands = false;
    return cmd;
}

void Listing::Complete(CmdList& cmd, std::vector<std::unique_ptr<action>>& actions)
{
    state_->complete = true;
    state_->groups.clear();

    if (!cmd.IsGood())
        return;

    const auto& buffer = cmd.GetBuffer(0);
    if (buffer.GetContentStatus() != Buffer::Status::Success)
        return;

    const nntp::linebuffer lines(buffer.Content(), buffer.GetContentLength());

    auto beg = lines.begin();
    auto end = lines.end();
    for (; beg != end; ++beg)
    {
        const auto& line = *beg;
        const auto& ret  = nntp::parse_group_list_item(line.start, line.length);
        if (!ret.first)
            continue;

        const auto& data = ret.second;
        NewsGroup group;
        group.last  = std::stoull(data.last);
        group.first = std::stoull(data.first);
        group.name  = data.name;
        group.size  = 0;

        // if the last field is less than the first field
        // then there are no articles in the group.
        if (group.last >= group.first)
            group.size = group.last - group.first + 1; // inclusive

        state_->groups.push_back(group);
    }
}
bool Listing::HasCommands() const
{ return state_->commands; }

float Listing::GetProgress() const
{ return state_->complete ? 100.0f : 0.0f; }

void Listing::Tick()
{
    std::lock_guard<std::mutex> lock(state_->mutex);

    if (!state_->progress_callback)
        return;
    if (state_->complete)
        return;
    if (state_->groups.empty())
        return;

    Progress progress;
    progress.groups  = std::move(state_->groups);
    state_->groups = std::vector<NewsGroup>();
    state_->progress_callback(progress);
}

void Listing::SetProgressCallback(const OnProgress& callback)
{ state_->progress_callback = callback; }

size_t Listing::NumGroups() const
{ return state_->groups.size(); }

const Listing::NewsGroup& Listing::GetGroup(size_t i) const
{
    ASSERT(i < state_->groups.size());
    return state_->groups[i];
}

// static
void Listing::ParseIntermediateBuffer(const Buffer& buff, bool compressed,
    std::shared_ptr<State> state)
{
    // this call is executed in whatever thread that is running the cmdlist

    // we don't know how to deal with compressed data.
    if (compressed)
        return;

    std::lock_guard<std::mutex> lock(state->mutex);
    if (state->discard)
        return;

    const nntp::linebuffer lines(buff.Head() + state->last_parse_offset,
        buff.GetSize() - state->last_parse_offset);
    auto beg = lines.begin();
    auto end = lines.end();
    if (beg == end)
        return;

    if (state->last_parse_offset == 0)
    {
        const auto& line = *beg;
        state->last_parse_offset = line.length;
        ++beg;
    }

    for (; beg != end; ++beg)
    {
        const auto& line = *beg;
        const auto& ret  = nntp::parse_group_list_item(line.start, line.length);
        if (!ret.first)
            continue;

        const auto& data = ret.second;
        NewsGroup group;
        group.last  = std::stoull(data.last);
        group.first = std::stoull(data.first);
        group.name  = data.name;
        group.size  = 0;
        if (group.last >= group.first)
            group.size = group.last - group.first + 1;

        state->groups.push_back(group);
        state->last_parse_offset += line.length;
    }
}

} // newsflash
