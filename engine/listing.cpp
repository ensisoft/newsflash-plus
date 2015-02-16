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


std::shared_ptr<cmdlist> listing::create_commands()
{
    std::shared_ptr<cmdlist> cmd(new cmdlist(cmdlist::listing{}));
    run_once_ = false;
    return cmd;
}

void listing::complete(cmdlist& cmd, 
    std::vector<std::unique_ptr<action>>& actions)
{
    if (cmd.is_canceled() || cmd.is_empty())
    {
        run_once_ = false;
        return;
    }

    auto& contents = cmd.get_buffers();
    ASSERT(!contents.empty());
    auto& listing  = contents[0];

    nntp::linebuffer lines(listing.content(), listing.content_length());

    auto beg = lines.begin();
    auto end = lines.end();
    for (; beg != end; ++beg)
    {
        const auto& line = *beg;
        const auto& ret  = nntp::parse_group(line.start, line.length);
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
