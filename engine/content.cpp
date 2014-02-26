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
#include <utility>
#include "content.h"
#include "bigfile.h"
#include "buffer.h"
#include "decoder.h"
#include "filesys.h"

namespace newsflash
{

content::content(std::string folder, std::string initial_file_name, std::unique_ptr<decoder> dec, bool overwrite) 
    : next_buffer_id_(0), 
      path_(std::move(folder)),
      name_(std::move((initial_file_name))),
      decoder_(std::move(dec)),
      overwrite_(overwrite)
{
    decoder_->on_error = std::bind(&content::on_error, this, std::placeholders::_1);    
    decoder_->on_info  = std::bind(&content::on_info, this, std::placeholders::_1);
    decoder_->on_write = std::bind(&content::on_write, this, 
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

#ifdef NEWSFLASH_DEBUG
    completed_ = false;
#endif
}

content::~content()
{
#ifdef NEWSFLASH_DEBUG
    assert(completed_);
#endif
}

void content::decode(std::shared_ptr<const buffer> buff)
{
    const auto id = buff->id();

    // if a buffer is out of sequence we'll need to copy it
    // and then deal with it later. this is because
    // it's not know if the decoding scheme can deal with out of order
    // buffers or not.
    if (id != next_buffer_id_)
    {
        const auto ret = buffers_.insert(std::make_pair(buff, id));
        if (!ret.second)
            throw std::runtime_error("duplicate content sequence");
    }

    decoder_->decode((const char*)buff->ptr() + buff->offset(),
        buff->size() - buff->offset());

    next_buffer_id_ = id + 1;
}


void content::cancel()
{
    decoder_->cancel();
    if (file_.is_open())
    {
        file_.close();
        bigfile::erase(path_ + "/" + name_);
    }
#ifdef NEWSFLASH_DEBUG
    completed_ = true;
#endif    
}

void content::finish()
{
    for (auto& pair : buffers_)
    {
        const auto& buff = pair.first;
        decoder_->decode((const char*)buff->ptr(), buff->size());
    }

    decoder_->finish();

    if (file_.is_open())
    {
        file_.flush();
        file_.close();
    }
#ifdef NEWSFLASH_DEBUG
    completed_ = true;
#endif
}

bool content::good() const
{
    return errors_.empty();
}

void content::on_error(const std::string& err)
{
    errors_.push_back(err);
}

void content::on_info(const decoder::info& info)
{
    const auto clean = fs::remove_illegal_filename_chars(info.name);
    if (!clean.empty())
        name_ = clean;
}

void content::on_write(const void* data, std::size_t len, std::size_t offset)
{
    if (!file_.is_open())
    {
        for (int i=0; i<11; ++i)
        {
            const std::string name = fs::name_file(i, name_);
            const std::string file = fs::join_path(path_, name_);

            if (!overwrite_ && bigfile::exists(file))
                continue;

            const std::error_code err = file_.create(file);
            if (err != std::error_code())
                throw std::system_error(err, "error opening file: " + file);

            name_ = name;
            break;
        }
    }

    stopwatch_timer io(watch_);

    file_.seek(offset);
    file_.write(data, len);
}

} // newsflash
