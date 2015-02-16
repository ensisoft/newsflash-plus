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

#include <functional>
#include <algorithm>
#include <atomic>

#include "ui/download.h"
#include "download.h"
#include "buffer.h"
#include "filesys.h"
#include "action.h"
#include "bigfile.h"
#include "bitflag.h"
#include "decode.h"
#include "format.h"
#include "settings.h"
#include "datafile.h"
#include "cmdlist.h"

namespace newsflash
{

download::download(std::vector<std::string> groups, std::vector<std::string> articles, std::string path, std::string name) : 
    groups_(std::move(groups)), articles_(std::move(articles)), 
    path_(std::move(path))
{
    name_        = fs::remove_illegal_filename_chars(name);
    num_commands_done_  = 0;
    num_commands_total_ = articles_.size();
    num_bytes_done_     = 0;
    overwrite_   = false;
    discardtext_ = false;
}

download::~download()
{}

std::shared_ptr<cmdlist> download::create_commands()
{
    if (articles_.empty())
        return nullptr;

    // take the next list of articles to be downloaded
    // if each article is about half a meg then this is
    // about 5 megs. if we're running on a 50mbit/s LAN
    // at full throttle it will take about a second to download
    //const std::size_t num_articles_per_cmdlist = 10;

    const std::size_t num_articles_per_cmdlist = 10;
    const std::size_t num_articles = std::min(articles_.size(), 
        num_articles_per_cmdlist);

    std::vector<std::string> next;
    std::copy(std::begin(articles_), std::begin(articles_) + num_articles,
        std::back_inserter(next));

    articles_.erase(std::begin(articles_), std::begin(articles_) + num_articles);

    cmdlist::messages m;
    m.groups  = groups_;
    m.numbers = std::move(next);

    auto cmd = std::make_shared<cmdlist>(std::move(m));
    return cmd;
}

void download::cancel()
{
    // there might be pending operations on the files (writes)
    // but if the task is being killed we simply reduce those 
    // operations to no-ops and then delete any files we created
    // once all the operations go away.
    for (auto& f : files_)
        f->discard_on_close();

    files_.clear();
}

void download::commit()
{
    // release our file handles so that we dont keep the file objects
    // open anymore needlessly. yet we keep the file objects around
    // since our other data is stored there.
    for (auto& f : files_)
        f->close();
}

void download::complete(action& act, std::vector<std::unique_ptr<action>>& next)
{
    // the action is either a decoding action or a datafile::write action.
    // for the write there's nothing we need to do.
    auto* dec = dynamic_cast<decode*>(&act);
    if (dec == nullptr)
        return;

    auto binary = dec->get_binary_data_move(); //std::move(*dec).get_binary_data();
    auto text   = dec->get_text_data_move(); //std::move(*dec).get_text_data();
    auto name   = dec->get_binary_name();

    if (!binary.empty())
    {
        if (name.empty())
            throw std::runtime_error("binary has no name");

        const auto size = dec->get_binary_size();

        // see comment in datafile about the +1 for offset
        const auto offset = dec->get_encoding() == encoding::yenc_multi ?
           dec->get_binary_offset() + 1 : 0;

        std::shared_ptr<datafile> file;

        auto it = std::find_if(std::begin(files_), std::end(files_),
            [=](const std::shared_ptr<datafile>& f) {
                return f->is_binary() && f->binary_name() == name;
            });
        if (it == std::end(files_))
        {
            file = std::make_shared<datafile>(path_, name, size, true, overwrite_);
            files_.push_back(file);
        } 
        else 
        {
            file = *it;
            assert(file->is_open());
        }

        std::unique_ptr<action> write(new datafile::write(offset, std::move(binary), file));
        next.push_back(std::move(write));
    }

    if (!text.empty() && !discardtext_)
    {
        std::shared_ptr<datafile> file;

        auto it = std::find_if(std::begin(files_), std::end(files_),
            [=](const std::shared_ptr<datafile>& f) {
                return !f->is_binary();
            });

        if (it == std::end(files_))
        {
            file = std::make_shared<datafile>(path_, name_, 0, false, overwrite_);
            files_.push_back(file);
        }
        else file = *it;

        std::unique_ptr<action> write(new datafile::write(0, std::move(text), file));
        next.push_back(std::move(write));
    }

    ++num_commands_done_;
}

void download::complete(cmdlist& cmd, std::vector<std::unique_ptr<action>>& next)
{
    auto& messages = cmd.get_commands();
    auto& contents = cmd.get_buffers();    

    for (std::size_t i=0; i<contents.size(); ++i)
    {
        auto& content = contents[i];
        auto& message = messages[i];
        auto status   = content.content_status();

        // if the article content was succesfully retrieved
        // create a decoding job and push into the output queue
        if (status == buffer::status::success)
        {
            std::unique_ptr<action> dec(new decode(std::move(content)));
            dec->set_affinity(action::affinity::any_thread);
            next.push_back(std::move(dec));
            continue;
        }

        ++num_commands_done_;
    }

    // all not yet processed messages go back into pending list
    std::copy(std::begin(messages) + contents.size(), std::end(messages),
        std::back_inserter(articles_));
}


void download::configure(const settings& s) 
{
    overwrite_   = s.overwrite_existing_files;
    discardtext_ = s.discard_text_content;
}

double download::completion() const 
{ 
    return 100.0 * double(num_commands_done_) / double(num_commands_total_);
}

} // newsflash

