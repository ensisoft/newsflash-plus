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
#include <fstream>
#include "listing.h"
#include "buffer.h"
#include "nntp.h"
#include "linebuffer.h"
#include "platform.h"

namespace newsflash
{

listing::io::io(std::string filename) : filename_(std::move(filename))
{}

void listing::io::prepare()
{}

void listing::io::receive(const buffer& buff)
{
    const nntp::linebuffer lines((const char*)buffer_payload(buff), 
        buffer_payload_size(buff));

    auto beg = lines.begin();
    auto end = lines.end();

    while (beg != end)
    {
        const auto& line = *beg;
        const auto& ret  = nntp::parse_group(line.start, line.length);
        if (ret.first)
        {
            const auto& data = ret.second;

            group_info group;
            group.size = 0;
            group.name = data.name;
            const std::uint64_t first = boost::lexical_cast<std::uint64_t>(data.first);
            const std::uint64_t last  = boost::lexical_cast<std::uint64_t>(data.last);

            // if the last field is less than the first field then
            // there are no articles in the group.
            if (last > first)
                group.size = last - first + 1; // inclusive

            groups_.push_back(std::move(group));
        }
        ++beg;
    }
}

void listing::io::cancel()
{}

void listing::io::flush()
{}

void listing::io::finalize()
{
    std::sort(groups_.begin(), groups_.end(), 
        [](const group_info& lhs, const group_info& rhs)
        {
            return lhs.name < rhs.name;
        });

    std::ofstream out;
    open_fstream(filename_, out);

    if (!out.is_open())
        throw std::system_error(std::error_code(errno, std::generic_category()), 
            "failed to open " + filename_);

    out << groups_.size() << std::endl;

    for (const auto& group : groups_)
    {
         out << group.name << "," << group.size;
         out << std::endl;
    }

    if (out.fail())
        throw std::system_error(std::error_code(errno, std::generic_category()),
            "io failure on " + filename_);
}

} // newsflash
