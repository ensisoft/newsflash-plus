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

#include <newsflash/warnpush.h>
#  include <boost/crc.hpp>
#include <newsflash/warnpop.h>

#include "decode.h"
#include "linebuffer.h"
#include "bodyiter.h"
#include "yenc.h"
#include "uuencode.h"

namespace newsflash
{

decode::decode(buffer&& data) : data_(std::move(data))
{
    binary_offset_ = 0;
    binary_size_   = 0;
    encoding_      = encoding::unknown;
}

decode::~decode()
{}

void decode::xperform()
{
    // iterate over the content line by line and inspect
    // every line untill we can identify a binary encoding.
    // we're assuming that data before and after binary content
    // is simply textual content such as is often the case with 
    // for example uuencoded images embedded in a text. 
    encoding enc = encoding::unknown;

    nntp::linebuffer lines(data_.content(), data_.content_length());
    nntp::linebuffer::iterator beg = lines.begin();
    nntp::linebuffer::iterator end = lines.end();

    std::size_t binary_start_offset = 0;
    while (beg != end && enc == encoding::unknown)
    {
        const auto line = *beg;
        enc = identify_encoding(line.start, line.length);
        if (enc != encoding::unknown) 
            break;

        binary_start_offset += line.length;            

        // save the text data before the binary in the text buffer
        if (line.length > 2 || !text_.empty())
            std::copy(line.start, line.start + line.length, std::back_inserter(text_));

        ++beg;
    }

    encoding_ = enc;

    std::size_t consumed = 0;
    const auto dataptr = data_.content() + binary_start_offset;
    const auto datalen = data_.content_length() - binary_start_offset;

    std::stringstream ss;
    ss << "\r\n";

    // decode the binary
    switch (enc)
    {
        // a simple case, a yenc encoded binary in that is only a single part
        case encoding::yenc_single:
            consumed = decode_yenc_single(dataptr, datalen);
            break;

        // a part of multi part(multi buffer) yenc encoded binary
        case encoding::yenc_multi:
            consumed = decode_yenc_multi(dataptr, datalen);
            break;

        // a uuencoded binary, typically a picture
        case encoding::uuencode_single:
            consumed = decode_uuencode_single(dataptr, datalen);
            break;

        // uunencoded binary split into several messages, not supported by the encoding,
        // but sometimes used to in case of larger pictures and such
        case encoding::uuencode_multi:
            consumed = decode_uuencode_multi(dataptr, datalen);
            break;

        // if we're unable to identify the encoding it could be something that we're not 
        // yet supporting such as MIME/base64.
        // in this case all the content is stored in the text buffer.
        case encoding::unknown:
           return;
    }

    if (text_.empty()) 
        return;
    
    if (!binary_name_.empty())
        ss << "[" << binary_name_ << "]";
    else ss << "[binary content]";
    ss << "\r\n";

    std::string s;
    ss >> s;
    std::copy(std::begin(s), std::end(s), std::back_inserter(text_));

    const auto textptr = dataptr + consumed;
    const auto textend = data_.content() + data_.content_length();
    std::copy(textptr, textend, std::back_inserter(text_));
}

std::size_t decode::decode_yenc_single(const char* data, std::size_t len)
{
    nntp::bodyiter beg(data, len);
    nntp::bodyiter end(data + len, 0);

    const auto header = yenc::parse_header(beg, end);
    if (!header.first)
        throw exception("broken or missing yenc header");

    std::vector<char> buff;
    buff.reserve(header.second.size);
    yenc::decode(beg, end, std::back_inserter(buff));

    const auto footer = yenc::parse_footer(beg, end);
    if (!footer.first)
        throw exception("broken or missing yenc footer");

    binary_        = std::move(buff);
    binary_name_   = header.second.name;
    binary_offset_ = 0;
    binary_size_   = binary_.size();

    if (footer.second.crc32)
    {
        boost::crc_32_type crc;
        crc.process_bytes(binary_.data(), binary_.size());
        if (footer.second.crc32 != crc.checksum())
            errors_.set(error::crc_mismatch);

        if (footer.second.size != header.second.size)
            errors_.set(error::size_mismatch);
        if (footer.second.size != binary_.size())
            errors_.set(error::size_mismatch);
    }
    return beg.position();
}

std::size_t decode::decode_yenc_multi(const char* data, std::size_t len)
{
    nntp::bodyiter beg(data, len);
    nntp::bodyiter end(data + len, 0);

    const auto header = yenc::parse_header(beg, end);
    if (!header.first)
        throw exception("broken or missing yenc header");

    const auto part = yenc::parse_part(beg, end);
    if (!part.first)
        throw exception("broken or missing yenc part header");

    // yenc uses 1 based offsets
    const auto offset = part.second.begin - 1;
    const auto size = part.second.end - offset;

    std::vector<char> buff;
    buff.reserve(size);
    yenc::decode(beg, end, std::back_inserter(buff));

    const auto footer = yenc::parse_footer(beg, end);
    if (!footer.first)
        throw exception("broken or missing yenc footer");

    binary_        = std::move(buff);
    binary_name_   = header.second.name;
    binary_offset_ = offset;
    binary_size_   = header.second.size;

    if (footer.second.pcrc32)
    {
        boost::crc_32_type crc;
        crc.process_bytes(binary_.data(), binary_.size());

        if (footer.second.pcrc32 != crc.checksum())
            errors_.set(error::crc_mismatch);

        if (size != binary_.size())
            errors_.set(error::size_mismatch);
    }

    return beg.position();
}

std::size_t decode::decode_uuencode_single(const char* data, std::size_t len)
{
    nntp::bodyiter beg(data, len);
    nntp::bodyiter end(data + len, 0);

    const auto header = uuencode::parse_begin(beg, end);
    if (!header.first)
        throw exception("broken or missing uuencode header");

    std::vector<char> buff;
    uuencode::decode(beg, end, std::back_inserter(buff));

    // this might fail if we're dealing with the start of a uuencoded binary
    // that is split into several parts. 
    // then there's no end in this chunk.
    uuencode::parse_end(beg, end);

    binary_        = std::move(buff);
    binary_name_   = header.second.file;
    binary_offset_ = 0;
    binary_size_   = 0;

    return beg.position();
}

std::size_t decode::decode_uuencode_multi(const char* data, std::size_t len)
{
    nntp::bodyiter beg(data, len);
    nntp::bodyiter end(data + len, 0);

    std::vector<char> buff;
    uuencode::decode(beg, end, std::back_inserter(buff));

    // see comments in uuencode_single
    uuencode::parse_end(beg, end);

    binary_        = std::move(buff);
    binary_offset_ = 0; // we have no idea about the offset, uuencode doesn't have it.
    binary_size_   = 0;

    return beg.position();
}

} // newsflash