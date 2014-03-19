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

#include <vector>
#include "nntp/bodyiter.h"
#include "uuencode_decoder.h"
#include "uuencode.h"

namespace newsflash
{

uuencode_decoder::uuencode_decoder() : has_header_(false)
{}

uuencode_decoder::~uuencode_decoder()
{}

std::size_t uuencode_decoder::decode(const void* data, std::size_t len)
{
    nntp::bodyiter beg((const char*)data);
    nntp::bodyiter end((const char*)data, len);

    if (!has_header_)
    {
        const auto& ret = uuencode::parse_begin(beg, end);
        if (!ret.first)
            throw decoder::exception("broken or missing header");

        if (on_info)
        {
            decoder::info info;
            info.size = 0;
            info.name = ret.second.file;
            on_info(info);
        }
        
        has_header_ = true;
    }

    std::vector<char> buff;
    uuencode::decode(beg, end, std::back_inserter(buff));
    
    if (!uuencode::parse_end(beg, end))
        throw decoder::exception("broken or missing end");

    on_write(buff.data(), buff.size(), 0, false);

    return distance(beg, end);
}

void uuencode_decoder::finish()
{}

} // newsflash
