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

#include "nntp.h"
#include "linebuffer.h"
#include "listing.h"
#include "buffer.h"
#include "cmdlist.h"
#include "assert.h"

namespace newsflash
{

std::shared_ptr<CmdList> Listing::CreateCommands()
{
    std::shared_ptr<CmdList> cmd(new CmdList(CmdList::Listing{}));
    ready_ = true;
    return cmd;
}

void Listing::Complete(CmdList& cmd, std::vector<std::unique_ptr<action>>& actions)
{
    if (cmd.IsCancelled() || cmd.IsEmpty())
        return;

    auto& listing  = cmd.GetBuffer(0);

    nntp::linebuffer lines(listing.content(), listing.content_length());

    auto beg = lines.begin();
    auto end = lines.end();
    for (; beg != end; ++beg)
    {
        const auto& line = *beg;
        const auto& ret  = nntp::parse_group_list_item(line.start, line.length);
        if (!ret.first)
            continue;

        const auto& data = ret.second;
        group g;
        g.last  = std::stoull(data.last);
        g.first = std::stoull(data.first);
        g.name  = data.name;
        g.size  = 0;

        // if the last field is less than the first field
        // then there are no articles in the group.
        if (g.last >= g.first)
            g.size = g.last - g.first + 1; // inclusive

        groups_.push_back(g);
    }
}

} // newsflash
