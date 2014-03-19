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
#include "nntp/linebuffer.h"
#include "download.h"
#include "buffer.h"
#include "filesys.h"
#include "yenc_single_decoder.h"
#include "yenc_multi_decoder.h"

namespace newsflash
{


download::download(std::string path, std::string name) : path_(std::move(path)), name_(std::move(name))
{}

void download::prepare()
{}

void download::receive(buffer buff, std::size_t id)
{
    const buffer::payload body(buff);

    const nntp::linebuffer lines(body.data(), body.size());

    auto beg = lines.begin();
    auto end = lines.end();
    while (beg != end)
    {
        const auto& line = *beg;
        const auto enc = identify_encoding(line.start, line.length);


        switch (enc)
        {
            case encoding::yenc_single:
            break;

            case encoding::yenc_multi:
            break;

            case encoding::uuencode_single:
            break;

            case encoding::uuencode_multi:
            break;

            default:
            break;
        }
    }
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

    }
}

download::content* download::find_by(encoding enc)
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

