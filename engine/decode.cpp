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

#include "newsflash/warnpush.h"
#  include <boost/crc.hpp>
#include "newsflash/warnpop.h"

#include "decode.h"
#include "linebuffer.h"
#include "bodyiter.h"
#include "yenc.h"
#include "uuencode.h"
#include "iso_8859_15.h"

namespace {

// yEnc encoding doesn't specify an encoding for the filenames,
// but I have a feeling that some bastards put utf-8 in there.
// so basically to figure out the filename we first see if it's a valid
// utf-8 sequence, if it is, use it as it is. Otherwise
// assume that the string is latin1 and convert to unicode and then encode into utf-8

std::string to_utf8(const std::string& str)
{
    bool success = false;
    utf8::decode(str, &success);
    if (success)
        return str;

    const auto& wide = newsflash::ISO_8859_15_to_unicode(str);
    return utf8::encode(wide);
}

} // namespace

namespace newsflash
{

DecodeJob::DecodeJob(const buffer& data) : data_(data)
{}

DecodeJob::DecodeJob(buffer&& data) : data_(std::move(data))
{}

std::string DecodeJob::describe() const
{
    return "DecodeJob";
}

void DecodeJob::xperform()
{
    // iterate over the content line by line and inspect
    // every line untill we can identify a binary encoding.
    // we're assuming that data before and after binary content
    // is simply textual content such as is often the case with
    // for example uuencoded images embedded in a text.
    newsflash::encoding enc = newsflash::encoding::unknown;

    nntp::linebuffer lines(data_.content(), data_.content_length());
    nntp::linebuffer::iterator beg = lines.begin();
    nntp::linebuffer::iterator end = lines.end();

    std::size_t binary_start_offset = 0;
    while (beg != end && enc == newsflash::encoding::unknown)
    {
        const auto line = *beg;
        enc = identify_encoding(line.start, line.length);
        if (enc != newsflash::encoding::unknown)
            break;

        binary_start_offset += line.length;

        // save the text data before the binary in the text buffer
        if (line.length > 2 || !text_.empty())
            std::copy(line.start, line.start + line.length, std::back_inserter(text_));

        ++beg;
    }

    std::size_t consumed = 0;
    const auto dataptr = data_.content() + binary_start_offset;
    const auto datalen = data_.content_length() - binary_start_offset;

    std::stringstream ss;
    ss << "\r\n";

    // DecodeJob the binary
    switch (enc)
    {
        // a simple case, a yenc encoded binary in that is only a single part
        case newsflash::encoding::yenc_single:
            consumed = decode_yenc_single(dataptr, datalen);
            break;

        // a part of multi part(multi buffer) yenc encoded binary
        case newsflash::encoding::yenc_multi:
            consumed = decode_yenc_multi(dataptr, datalen);
            break;

        // a uuencoded binary, typically a picture
        case newsflash::encoding::uuencode_single:
            consumed = decode_uuencode_single(dataptr, datalen);
            break;

        // uunencoded binary split into several messages, not supported by the encoding,
        // but sometimes used to in case of larger pictures and such
        case newsflash::encoding::uuencode_multi:
            consumed = decode_uuencode_multi(dataptr, datalen);
            break;

        // if we're unable to identify the encoding it could be something that we're not
        // yet supporting such as MIME/base64.
        // in this case all the content is stored in the text buffer.
        case newsflash::encoding::unknown:
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

std::size_t DecodeJob::decode_yenc_single(const char* data, std::size_t len)
{
    nntp::bodyiter beg(data, len);
    nntp::bodyiter end(data + len, 0);

    const auto header = yenc::parse_header(beg, end);
    if (!header.first)
        throw Exception("broken or missing yenc header");

    std::vector<char> buff;
    buff.reserve(header.second.size);
    yenc::decode(beg, end, std::back_inserter(buff));

    const auto footer = yenc::parse_footer(beg, end);
    if (!footer.first)
        throw Exception("broken or missing yenc footer");

    binary_        = std::move(buff);
    binary_name_   = to_utf8(header.second.name);
    binary_offset_ = 0;
    binary_size_   = binary_.size();
    flags_.set(Flags::Multipart, false);
    flags_.set(Flags::FirstPart, true);
    flags_.set(Flags::LastPart, true);
    flags_.set(Flags::HasOffset, false);
    encoding_ = Encoding::yEnc;

    if (footer.second.crc32)
    {
        boost::crc_32_type crc;
        crc.process_bytes(binary_.data(), binary_.size());
        if (footer.second.crc32 != crc.checksum())
            errors_.set(Error::CrcMismatch);

        if (footer.second.size != header.second.size)
            errors_.set(Error::SizeMismatch);
        if (footer.second.size != binary_.size())
            errors_.set(Error::SizeMismatch);
    }
    return beg.position();
}

std::size_t DecodeJob::decode_yenc_multi(const char* data, std::size_t len)
{
    nntp::bodyiter beg(data, len);
    nntp::bodyiter end(data + len, 0);

    const auto header = yenc::parse_header(beg, end);
    if (!header.first)
        throw Exception("broken or missing yenc header");

    const auto part = yenc::parse_part(beg, end);
    if (!part.first)
        throw Exception("broken or missing yenc part header");

    // yenc uses 1 based offsets
    const auto offset = part.second.begin - 1;
    const auto size = part.second.end - offset;

    std::vector<char> buff;
    buff.reserve(size);
    yenc::decode(beg, end, std::back_inserter(buff));

    const auto footer = yenc::parse_footer(beg, end);
    if (!footer.first)
        throw Exception("broken or missing yenc footer");

    binary_        = std::move(buff);
    binary_name_   = to_utf8(header.second.name);
    binary_offset_ = offset;
    binary_size_   = header.second.size;
    flags_.set(Flags::Multipart, true);
    flags_.set(Flags::FirstPart, header.second.part == 1);
    flags_.set(Flags::LastPart, header.second.part == header.second.total);
    flags_.set(Flags::HasOffset, true);
    encoding_ = Encoding::yEnc;

    if (footer.second.pcrc32)
    {
        boost::crc_32_type crc;
        crc.process_bytes(binary_.data(), binary_.size());

        if (footer.second.pcrc32 != crc.checksum())
            errors_.set(Error::CrcMismatch);

        if (size != binary_.size())
            errors_.set(Error::SizeMismatch);
    }

    return beg.position();
}

std::size_t DecodeJob::decode_uuencode_single(const char* data, std::size_t len)
{
    nntp::bodyiter beg(data, len);
    nntp::bodyiter end(data + len, 0);

    const auto header = uuencode::parse_begin(beg, end);
    if (!header.first)
        throw Exception("broken or missing uuencode header");

    std::vector<char> buff;
    uuencode::decode(beg, end, std::back_inserter(buff));

    // this might fail if we're dealing with the start of a uuencoded binary
    // that is split into several parts.
    // then there's no end in this chunk.
    const bool parse_end_success = uuencode::parse_end(beg, end);

    binary_        = std::move(buff);
    binary_name_   = to_utf8(header.second.file);
    binary_offset_ = 0;
    binary_size_   = 0;
    flags_.set(Flags::FirstPart, true);
    flags_.set(Flags::HasOffset, false);
    encoding_ = Encoding::UUEncode;
    if (parse_end_success)
    {
        flags_.set(Flags::LastPart, true);
        flags_.set(Flags::Multipart, false);
    }
    else
    {
        flags_.set(Flags::LastPart, false);
        flags_.set(Flags::Multipart, true);
    }

    return beg.position();
}

std::size_t DecodeJob::decode_uuencode_multi(const char* data, std::size_t len)
{
    nntp::bodyiter beg(data, len);
    nntp::bodyiter end(data + len, 0);

    std::vector<char> buff;
    uuencode::decode(beg, end, std::back_inserter(buff));

    // see comments in uuencode_single
    const bool parse_end_success = uuencode::parse_end(beg, end);

    binary_        = std::move(buff);
    binary_offset_ = 0; // we have no idea about the offset, uuencode doesn't have it.
    binary_size_   = 0;
    flags_.set(Flags::Multipart, true);
    flags_.set(Flags::FirstPart, false);
    flags_.set(Flags::HasOffset, false);
    flags_.set(Flags::LastPart, parse_end_success);
    encoding_ = Encoding::UUEncode;
    return beg.position();
}

} // newsflash