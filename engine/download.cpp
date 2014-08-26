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
#include "yenc_single_decoder.h"
#include "yenc_multi_decoder.h"
#include "uuencode_decoder.h"

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

} // 

namespace newsflash
{

download::download(std::string path, std::string name) : path_(std::move(path)), name_(std::move(name)), overwrite_(true), keeptext_(false)
{}

void download::overwrite(bool val)
{
    overwrite_ = val;
}

void download::keeptext(bool val)
{
    keeptext_ = val;
}

void download::prepare()
{}

void download::receive(buffer&& buff, std::size_t id)
{
    // buffer::payload body(buff);

    // // iterate over the data line by line and inspect
    // // every line untill we can identify a binary content
    // // encoded with some encoding scheme. then if data remains
    // // in the buffer we continue processing it line by line
    // // repeating the same inspection
    // // todo: MIME and base64 based encoding.

    // while (!body.empty())
    // {
    //     const nntp::linebuffer lines(body.data(), body.size());
    //     const auto& iter = lines.begin();
    //     const auto& line = *iter;

    //     download::content* content = nullptr;

    //     const auto enc = identify_encoding(line.start, line.length);
    //     switch (enc)
    //     {
    //         case encoding::yenc_single:
    //             {
    //                 download::content yenc;
    //                 yenc.size  = 0;
    //                 yenc.enc   = encoding::yenc_single;
    //                 yenc.codec.reset(new yenc_single_decoder);
    //                 content = bind_content(std::move(yenc));
    //             }
    //             break;

    //         case encoding::yenc_multi:
    //             content = find_content(encoding::yenc_multi);
    //             break;

    //         case encoding::uuencode_single:
    //             {
    //                 download::content uu;
    //                 uu.size = 0;
    //                 uu.enc  = encoding::uuencode_single;
    //                 uu.codec.reset(new uuencode_decoder);
    //                 content = bind_content(std::move(uu));
    //             }
    //             break;

    //             // uuencoding scheme doesn't officially cater for messages split
    //             // into several articles, however in practice it does happen, typically
    //             // with large images (larger than maximum single nntp article size ~800kb).
    //             // if we find a headerless uuencoded data we simply assume that it belongs
    //             // to some previous uuencoded data blob.
    //         case encoding::uuencode_multi:
    //             content = find_content(encoding::uuencode_single);
    //             break;

    //         case encoding::unknown:
    //             {
    //                 // if we're not keeping text then simply discard this line
    //                 if (!keeptext_)
    //                     break;

    //                 content = find_content(encoding::unknown);
    //                 if (!content)
    //                 {
    //                     download::content text;
    //                     text.size = 0;
    //                     text.enc  = encoding::unknown;
    //                     text.codec.reset(new text_decoder);
    //                     content = bind_content(std::move(text));
    //                 }
    //             }
    //             break;
    //     }
    //     if (!content)
    //         throw std::runtime_error("no such content found!");

    //     decoder& dec = *content->codec;

    //     const auto consumed = dec.decode(body.data(), body.size());

    //     body.crop(consumed);
    // }
}

void download::cancel()
{
    for (auto& content : contents_)
    {
        bigfile& big = content.file;
        if (big.is_open())
        {
            big.close();
            bigfile::erase(fs::joinpath(path_, content.name));
        }
    }
}

void download::flush()
{}

void download::finalize()
{
    for (auto& content : contents_)
    {
        decoder& dec = *content.codec;
        dec.finish();
    }
}

download::content* download::find_content(encoding enc)
{
    const auto it = std::find_if(contents_.begin(), contents_.end(), 
        [&](const content& c)
        {
            return c.enc == enc;
        });
    if (it == contents_.end())
        return nullptr;

    return &(*it);
}

download::content* download::bind_content(download::content&& content)
{
    decoder& dec = *content.codec;

    dec.on_info = std::bind(&download::on_info, this, 
        std::placeholders::_1, std::ref(content));
    dec.on_write = std::bind(&download::on_write, this, 
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4,
        std::ref(content));
    dec.on_error = std::bind(&download::on_error, this,
        std::placeholders::_1, std::placeholders::_2, std::ref(content));

    contents_.push_back(std::move(content));
    return &contents_.back();
}

void download::on_info(const decoder::info& info, download::content& content)
{
    const auto name = fs::remove_illegal_filename_chars(info.name);
    if (!name.empty())
        content.name = name;
    if (info.size)
        content.size = info.size;
}

void download::on_write(const void* data, std::size_t size, std::size_t offset, bool has_offset, download::content& content)
{
    bigfile& big = content.file;

    if (!big.is_open())
    {
        // try to open the file multiple times in case 
        // the filename we're trying to use already exists.
        for (int i=0; i<10; ++i)
        {
            const auto& name = fs::filename(i, content.name);
            const auto& file = fs::joinpath(path_, name);
            if (!overwrite_ && bigfile::exists(file))
                continue;

            const auto error = big.create(file);
            if (error != std::error_code())
                throw std::system_error(error, "error creating file: " + file);
            if (content.size)
            {
                const auto error = bigfile::resize(file, content.size);
                if (error != std::error_code())
                    throw std::system_error(error, "error resizing file: " + file);
            }
            content.name = name;
            content.file = std::move(big);
            break;
        }
    }
    if (!big.is_open())
        throw std::runtime_error("unable to create files at:" + path_);    

    if (has_offset)
    {
        big.seek(offset);
    }

    big.write(data, size);
}

void download::on_error(decoder::error error, const std::string& what, download::content& content)
{
    content.errors.push_back(what);
}

} // newsflash

