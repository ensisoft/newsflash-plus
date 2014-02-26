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

#include "yenc_multi_decoder.h"
#include "yenc.h"
#include "buffer.h"
#include "bodyiter.h"

namespace newsflash
{
yenc_multi_decoder::yenc_multi_decoder()
{}

yenc_multi_decoder::~yenc_multi_decoder()
{}

void yenc_multi_decoder::decode(const char* data, std::size_t len)
{
    nntp::bodyiter beg(data);
    nntp::bodyiter end(data + len);

    const auto header = yenc::parse_header(beg, end);
    if (!header.first)
        throw decoder::exception("yenc: broken or missing header");

    const auto part = yenc::parse_part(beg, end);
    if (!part.first)
        throw decoder::exception("yenc: broken or missing part header");

    std::vector<char> buff;

}

void yenc_multi_decoder::finish()
{

}

void yenc_multi_decoder::cancel()
{}

} // newsflash
