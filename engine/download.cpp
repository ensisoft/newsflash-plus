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
#  include "engine.pb.h"
#include "newsflash/warnpop.h"

#include <functional>
#include <algorithm>
#include <atomic>

#include "assert.h"
#include "download.h"
#include "buffer.h"
#include "filesys.h"
#include "action.h"
#include "bigfile.h"
#include "bitflag.h"
#include "decode.h"
#include "format.h"
#include "datafile.h"
#include "cmdlist.h"

namespace newsflash
{

Download::Download(
    const std::vector<std::string>& groups,
    const std::vector<std::string>& articles,
    const std::string& path,
    const std::string& name)
    : groups_(groups)
    , articles_(articles)
    , path_(path)
{
    name_         = fs::remove_illegal_filename_chars(name);
    decode_jobs_  = articles_.size();
}

Download::Download()
{}

std::shared_ptr<CmdList> Download::CreateCommands()
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

    CmdList::Messages m;
    m.groups  = groups_;
    m.numbers = std::move(next);

    auto cmd = std::make_shared<CmdList>(std::move(m));
    return cmd;
}

void Download::Cancel()
{
    // there might be pending operations on the files (writes)
    // but if the task is being killed we simply reduce those
    // operations to no-ops and then delete any files we created
    // once all the operations go away.
    for (auto& f : files_)
        f->DiscardOnClose();

    files_.clear();
}

void Download::Commit()
{
    if (!stash_.empty())
    {
        std::shared_ptr<DataFile> file = create_file(stash_name_, 0);

        // collect the stuff from the stash.
        for (auto& p : stash_)
        {
            if (!p) continue;

            std::unique_ptr<action> write = file->Write(0, std::move(*p), callback_);
            write->perform();
            p.reset();
        }
        stash_.clear();
    }

    // release our file handles so that we dont keep the file objects
    // open anymore needlessly. yet we keep the file objects around
    // since our other data is stored there.
    for (auto& f : files_)
        f->Close();
}

void Download::Complete(action& act, std::vector<std::unique_ptr<action>>& next)
{
    // the action is either a decoding action or a datafile::write action.
    // for the write there's nothing we need to do.
    auto* dec = dynamic_cast<DecodeJob*>(&act);
    if (dec == nullptr)
        return;

    const auto err = dec->GetErrors();
    if (err.test(DecodeJob::Error::CrcMismatch))
        errors_.set(Task::Error::CrcMismatch);
    if (err.test(DecodeJob::Error::SizeMismatch))
        errors_.set(Task::Error::SizeMismatch);

    auto binary = dec->GetBinaryDataMove(); //std::move(*dec).get_binary_data();
    auto text   = dec->GetTextDataMove(); //std::move(*dec).get_text_data();
    auto name   = dec->GetBinaryName();

    // process the binary data.
    if (!binary.empty())
    {
        const auto enc = dec->GetEncoding();
        if (enc == DecodeJob::Encoding::yEnc)
        {
            // see comment in datafile about the +1 for offset
            const auto offset = dec->HasOffset() ?
                dec->GetBinaryOffset() + 1 : 0;
            const auto size = dec->GetBinarySize();

            std::shared_ptr<DataFile> file = create_file(name, size);
            std::unique_ptr<action> write  = file->Write(offset, std::move(binary), callback_);
            next.push_back(std::move(write));
        }
        else if (enc == DecodeJob::Encoding::UUEncode)
        {
            // uuencode is a cumbersome encoding scheme. it's used mostly for pictures
            // and large pictures are split into multiple posts. however the encoding itself
            // provides no means for identifying the order of chunks. so basically we can
            // only encoding a maximum of 3 part binary properly since we can identify
            // the beginning and ending chunks. if a post has more parts than that the
            // ordering of the parts between begin and end is unknown and we have to hope
            // that the subject line convention has given us the ordering.
            // the subject line based ordering is then implied here through the order
            // of the article ids and by queuing the decoding actions to a single thread
            // so that the same ordering is maintained throughout.

            if (dec->IsMultipart())
            {
                if (stash_.empty())
                    stash_.resize(decode_jobs_);

                if (dec->IsFirstPart())
                {
                    std::unique_ptr<stash> s(new stash(std::move(binary)));
                    stash_[0]   = std::move(s);
                    stash_name_ = name;
                }
                else if (dec->IsLastPart())
                {
                    std::unique_ptr<stash> s(new stash(std::move(binary)));
                    stash_[decode_jobs_ - 1] = std::move(s);
                }
                else
                {
                    std::size_t index;
                    for (index=1; index<decode_jobs_ - 1; ++index)
                    {
                        auto& p = stash_[index];
                        if (!p)
                            break;
                    }
                    std::unique_ptr<stash> s(new stash(std::move(binary)));
                    stash_[index] = std::move(s);
                }
            }
            else
            {
                std::shared_ptr<DataFile> file = create_file(name, 0);
                std::unique_ptr<action> write  = file->Write(0, std::move(binary), callback_);
                next.push_back(std::move(write));
            }
        }
        else
        {
            throw std::runtime_error("Unsupported binary encoding");
        }
    }

    // process the text data unless it's to be discarded.
    if (!text.empty() && !discardtext_)
    {
        std::shared_ptr<DataFile> file;

        auto it = std::find_if(std::begin(files_), std::end(files_),
            [=](const std::shared_ptr<DataFile>& f) {
                return !f->IsBinary();
            });

        if (it == std::end(files_))
        {
            file = std::make_shared<DataFile>(path_, name_ + ".txt", 0, false, overwrite_);
            files_.push_back(file);
        }
        else file = *it;

        std::unique_ptr<action> write = file->Write(0, std::move(text), callback_);
        next.push_back(std::move(write));
    }
}

void Download::Complete(CmdList& cmd, std::vector<std::unique_ptr<action>>& next)
{
    auto& messages = cmd.GetCommands();
    auto& contents = cmd.GetBuffers();

    // yenc encoding based files have offsets so they can
    // run in parallel and complete in any order and then write
    // in any order to the file.
    const bool yenc = name_.find("yEnc") != std::string::npos;
    const auto affinity = yenc ?
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
            std::unique_ptr<DecodeJob> dec(new DecodeJob(std::move(content)));
            dec->set_affinity(affinity);
            //dec->set_decoding_id(decode_index_);
            next.push_back(std::move(dec));
        }
    }

    // all not yet processed messages go back into pending list
    std::copy(std::begin(messages) + contents.size(), std::end(messages),
        std::back_inserter(articles_));
}


void Download::Configure(const Settings& settings)
{
    overwrite_   = settings.overwrite_existing_files;
    discardtext_ = settings.discard_text_content;
}

bool Download::HasCommands() const
{
    return !articles_.empty();
}

std::size_t Download::MaxNumActions() const
{
    // 2 actions per each article. decode and write.
    return articles_.size() * 2;
}

bitflag<Task::Error> Download::GetErrors() const
{
    return errors_;
}

void Download::Pack(data::TaskState& data) const
{
    auto* ptr = data.mutable_download();

    ptr->set_num_decode_jobs(decode_jobs_);

    for (const auto& group : groups_)
    {
        ptr->add_group(group);
    }
    for (const auto& article : articles_)
    {
        ptr->add_article(article);
    }
    for (const auto& file : files_)
    {
        auto* file_data = ptr->add_file();
        file_data->set_filename(file->GetFileName());
        file_data->set_filepath(file->GetFilePath());
        file_data->set_dataname(file->GetBinaryName());
        file_data->set_is_binary(file->IsBinary());
    }

    for (size_t i=0; i<stash_.size(); ++i)
    {
        const auto& stash = *stash_[i];
        std::string str;
        std::copy(std::begin(stash), std::end(stash), std::back_inserter(str));
        auto* stash_data = ptr->add_stash();
        stash_data->set_sequence(i);
        stash_data->set_data(str);
    }
}

void Download::Load(const data::TaskState& data)
{
    ASSERT(data.has_download());

    const auto& ptr = data.download();

    decode_jobs_ = ptr.num_decode_jobs();

    // load the groups and article / message names
    for (int i=0; i<ptr.group_size(); ++i)
        groups_.push_back(ptr.group(i));
    for (int i=0; i<ptr.article_size(); ++i)
        articles_.push_back(ptr.article(i));

    // load the files that we're working on.
    for (int i=0; i<ptr.file_size(); ++i)
    {
        const auto& file_data = ptr.file(i);
        const auto& path = file_data.filepath();
        const auto& name = file_data.filename();
        const auto& binary_name = file_data.dataname();
        const bool is_binary = file_data.is_binary();
        auto file = std::make_shared<DataFile>(path, name, binary_name, is_binary);
        files_.push_back(file);
    }

    if (ptr.stash_size())
    {
        stash_.resize(decode_jobs_);
    }

    for (int i=0; i<ptr.stash_size(); ++i)
    {
        const auto& stash_data = ptr.stash(i);
        const auto index = stash_data.sequence();
        const auto& str  = stash_data.data();
        std::unique_ptr<stash> s(new stash);
        std::copy(std::begin(str), std::end(str), std::back_inserter(*s));
        stash_[index] = std::move(s);
    }
}

std::shared_ptr<DataFile> Download::create_file(const std::string& name, std::size_t assumed_size)
{
    if (name.empty())
        throw std::runtime_error("binary has no name");

    std::shared_ptr<DataFile> file;

    auto it = std::find_if(std::begin(files_), std::end(files_),
        [=](const std::shared_ptr<DataFile>& f) {
            return f->IsBinary() && f->GetBinaryName() == name;
        });
    if (it == std::end(files_))
    {
        file = std::make_shared<DataFile>(path_, name, assumed_size, true, overwrite_);
        files_.push_back(file);
    }
    else
    {
        file = *it;
        assert(file->IsOpen());
    }
    return file;
}

} // newsflash

