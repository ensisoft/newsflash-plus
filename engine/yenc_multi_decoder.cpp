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

#include "nntp/bodyiter.h"
#include "yenc_multi_decoder.h"
#include "yenc.h"

namespace engine
{

yenc_multi_decoder::yenc_multi_decoder() 
    : crc_value_(0), part_count_(0), part_(0), binary_size_(0), size_(0), has_header_(false)
{}

yenc_multi_decoder::~yenc_multi_decoder()
{}

std::size_t yenc_multi_decoder::decode(const void* data, std::size_t len)
{
    nntp::bodyiter beg((const char*)data);
    nntp::bodyiter end((const char*)data, len);

    const auto header = yenc::parse_header(beg, end);
    if (!header.first)
        throw decoder::exception("broken or missing header");

    const auto part = yenc::parse_part(beg, end);
    if (!part.first)
        throw decoder::exception("broken or missing part header");

    //  yenc uses 1 based offsets
    const std::size_t part_offset = part.second.begin -1;
    const std::size_t part_size   = part.second.end - part_offset;

    std::vector<char> buff;
    buff.reserve(part_size);
    yenc::decode(beg, end, std::back_inserter(buff));

    const auto footer = yenc::parse_footer(beg, end);
    if (!footer.first)
        throw decoder::exception("broken or missing footer");

    if (!has_header_)
    {
        part_count_  = header.second.total;
        binary_size_ = header.second.size;
        crc_value_   = footer.second.crc32;

        if (on_info)
        {
            decoder::info info;
            info.name = header.second.name;
            info.size = header.second.size;
            on_info(info);
        }
    }

    if (on_error)
    {
        if (crc_value_)
            crc_.process_bytes(buff.data(), buff.size());

        if (footer.second.pcrc32)
        {
            boost::crc_32_type crc;
            crc.process_bytes(buff.data(), buff.size());
            if (footer.second.pcrc32 != crc.checksum())
                on_error(error::crc, "part checksum mismatch");
        }

        if (part_size != buff.size())
            on_error(error::size, "size mismatch between buffer size and size in part header");
    }

    has_header_ = true;
    part_ += 1;
    size_ += buff.size();

    on_write(buff.data(), buff.size(), part_offset, true);    

    return len - distance(beg, end);
}

void yenc_multi_decoder::finish()
{
    if (!on_error)
        return;

    if (crc_value_)
    {
        if (crc_.checksum() != crc_value_)
            on_error(error::crc, "final checksum mismatch");
    }

    if (part_ != part_count_)
        on_error(error::partial_content, "missing parts");

    if (size_ != binary_size_)
        on_error(error::size, "size mismatch between binary size and encoded size");

}

} // engine
