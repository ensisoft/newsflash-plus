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
#  include <third_party/nlohmann/json.hpp>
#  include <third_party/base64/base64.h>
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
    const std::string& name,
    bool ignore_yenc_filename)
    : groups_(groups)
    , articles_(articles)
    , path_(path)
    , ignore_yenc_filename_(ignore_yenc_filename)
{
    name_         = fs::remove_illegal_filename_chars(name);
    num_decode_jobs_   = articles_.size();
    num_actions_total_ = articles_.size() * 2; // decode + write
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
    num_actions_ready_++;

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

            // ignore_yenc_filename flag is a workaround for obfuscation
            // where each yEnc header has a random garbage filename.
            // without this flag we'd create a separate file for each yEnc junk.
            // so this flag effectively forces all data to be written out to
            // a single file only.
            if (dec->IsMultipart() && ignore_yenc_filename_)
                name = name_;

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
                    stash_.resize(num_decode_jobs_);

                if (dec->IsFirstPart())
                {
                    std::unique_ptr<stash> s(new stash(std::move(binary)));
                    stash_[0]   = std::move(s);
                    stash_name_ = name;
                }
                else if (dec->IsLastPart())
                {
                    std::unique_ptr<stash> s(new stash(std::move(binary)));
                    stash_[num_decode_jobs_ - 1] = std::move(s);
                }
                else
                {
                    std::size_t index;
                    for (index=1; index<num_decode_jobs_ - 1; ++index)
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
    // if the cmdlist has failed there won't be any content
    // we want to handle. all buffers have None statuses.
    if (!cmd.IsGood())
        return;

    // yenc encoding based files have offsets so they can
    // run in parallel and complete in any order and then write
    // in any order to the file.
    const bool yenc = name_.find("yEnc") != std::string::npos;
    const auto affinity = yenc ?
        action::affinity::any_thread :
        action::affinity::single_thread;

    for (std::size_t i=0; i<cmd.NumBuffers(); ++i)
    {
        auto& buffer = cmd.GetBuffer(i);
        const auto status  = buffer.GetContentStatus();
        const auto command = cmd.GetCommand(i);

        // if the article content was succesfully retrieved
        // create a decoding job and push into the output queue
        if (status == Buffer::Status::Success)
        {
            std::unique_ptr<DecodeJob> dec(new DecodeJob(std::move(buffer)));
            dec->set_affinity(affinity);
            next.push_back(std::move(dec));
        }
        else if (status == Buffer::Status::None)
        {
            // if the article was not yet retrieved (perhaps the cmdlist was cancelled)
            // put the article id back into the articles list so that
            // we can pack state properly if needed.
            articles_.push_back(command);
        }
    }
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

bool Download::HasProgress() const
{ return true; }

float Download::GetProgress() const
{
    return float(num_actions_ready_) / float(num_actions_total_) * 100.0f;
}

bitflag<Task::Error> Download::GetErrors() const
{
    return errors_;
}

void Download::Pack(std::string* data) const
{
    nlohmann::json json;

    uint32_t flags = 0;
    if (ignore_yenc_filename_)
        flags |= 1 << 0;
    if (overwrite_)
        flags |= 1 << 1;
    if (discardtext_)
        flags |= 1 << 2;

    json["path"] = path_;
    json["name"] = name_;
    json["stash_name"] = stash_name_;
    json["error_bits"] = errors_.value();
    json["num_decode_jobs"] = num_decode_jobs_;
    json["num_actions_total"] = num_actions_total_;
    json["num_actions_ready"] = num_actions_ready_;
    json["flags"] = flags;
    json["groups"] = groups_;
    json["articles"] = articles_;

    for (const auto& file : files_)
    {
        nlohmann::json json_f;
        json_f["name"] = file->GetFileName();
        json_f["path"] = file->GetFilePath();
        json_f["binary_name"] = file->GetBinaryName();
        json_f["is_binary"] = file->IsBinary();
        json["files"].push_back(json_f);
    }

    for (size_t i=0; i<stash_.size(); ++i)
    {
        // the stash is a binary buffer of content that we haven't yet been able to
        // write out. base64 encode it!
        const auto& stash = *stash_[i];
        std::string binary;
        std::copy(std::begin(stash), std::end(stash), std::back_inserter(binary));

        const std::string& base64 = base64::Encode(binary);
        nlohmann::json json_s;
        json_s["sequence_number"] = i;
        json_s["data"] = base64;
        json["stashes"].push_back(json_s);
    }
    *data = json.dump(2);
}

void Download::Load(const std::string& data)
{
    const nlohmann::json& json = nlohmann::json::parse(data);

    const unsigned error_bits = json["error_bits"];
    const unsigned flag_bits = json["flags"];

    path_ = json["path"];
    name_ = json["name"];
    stash_name_ = json["stash_name"];
    errors_.set_from_value(error_bits);
    num_decode_jobs_ = json["num_decode_jobs"];
    num_actions_total_ = json["num_actions_total"];
    num_actions_ready_ = json["num_actions_ready"];

    // load the groups and article / message names
    articles_ = json["articles"].get<std::vector<std::string>>();
    groups_   = json["groups"].get<std::vector<std::string>>();

    const std::uint32_t flags = flag_bits; //ptr.flags();
    ignore_yenc_filename_ = (flags & (1<<0));
    overwrite_ = (flags & (1<<1));
    discardtext_ = (flags & (1<<2));

    // load the files that we're working on.
    if (json.contains("files"))
    {
        for (const auto& json_f : json["files"].items())
        {
            const auto& value = json_f.value();
            const std::string& path = value["path"];
            const std::string& name = value["name"];
            const std::string& binary_name = value["binary_name"];
            const bool is_binary = value["is_binary"];
            auto file = std::make_shared<DataFile>(path, name, binary_name, is_binary);
            files_.push_back(file);
        }
    }

    if (json.contains("stashes"))
    {
        for (const auto& json_s : json["stashes"].items())
        {
            const auto& value = json_s.value();
            const unsigned sequence_number = value["sequence_number"];
            const std::string& base64 = value["data"];
            const std::string& binary = base64::Decode(base64);
            std::unique_ptr<stash> stash(new Download::stash);
            std::copy(std::begin(binary), std::end(binary), std::back_inserter(*stash));

            if (stash_.empty())
                stash_.resize(num_decode_jobs_);
            stash_[sequence_number] = std::move(stash);
        }
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

