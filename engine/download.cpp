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
    groups_(std::move(groups)), articles_(std::move(articles)),  path_(std::move(path))
{
    name_        = fs::remove_illegal_filename_chars(name);
    overwrite_   = false;
    discardtext_ = false;
    yenc_        = false;

    const auto pos = name.find("yEnc");
    if (pos != std::string::npos)
        yenc_ = true;
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

    auto beg = std::begin(articles_);
    auto end = std::begin(articles_);
    std::advance(end, num_articles);

    std::vector<std::string> next;
    std::copy(beg, end, std::back_inserter(next));

    articles_.erase(beg, end);

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
        const auto enc = dec->get_encoding();

        // see comment in datafile about the +1 for offset
        const auto offset = enc == encoding::yenc_multi ?
           dec->get_binary_offset() + 1 : 0;

        const auto size = dec->get_binary_size();

        if (name.empty())
        {
            if (enc != encoding::uuencode_multi)
                throw std::runtime_error("binary has no name");

            if (files_.empty())
            {
                std::copy(std::begin(binary), std::end(binary), std::back_inserter(stash_));
                return;
            }
            else
            {
                name = files_.front()->binary_name();
            }
        }

        std::copy(std::begin(stash_), std::end(stash_), std::back_inserter(binary));
        stash_.clear();

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
            file = std::make_shared<datafile>(path_, name_ + ".txt", 0, false, overwrite_);
            files_.push_back(file);
        }
        else file = *it;

        std::unique_ptr<action> write(new datafile::write(0, std::move(text), file));
        next.push_back(std::move(write));
    }
}

void download::complete(cmdlist& cmd, std::vector<std::unique_ptr<action>>& next)
{
    auto& messages = cmd.get_commands();
    auto& contents = cmd.get_buffers();    

    const auto affinity = yenc_ ? 
        action::affinity::any_thread :
        action::affinity::single_thread;

    for (std::size_t i=0; i<contents.size(); ++i)
    {
        auto& content = contents[i];
        auto status   = content.content_status();

        // if the article content was succesfully retrieved
        // create a decoding job and push into the output queue
        if (status == buffer::status::success)
        {
            std::unique_ptr<action> dec(new decode(std::move(content)));
            dec->set_affinity(affinity);
            next.push_back(std::move(dec));
        }
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

bool download::has_commands() const
{
    return !articles_.empty(); 
}

std::size_t download::max_num_actions() const 
{
    // 2 actions per each article. decode and write.
    return articles_.size() * 2;
}

} // newsflash

