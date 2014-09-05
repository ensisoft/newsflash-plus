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

#include <newsflash/warnpush.h>
#  include <boost/crc.hpp>
#include <newsflash/warnpop.h>

#include <functional>
#include <algorithm>
#include <atomic>

#include "linebuffer.h"
#include "bodyiter.h"
#include "download.h"
#include "buffer.h"
#include "filesys.h"
#include "logging.h"
#include "action.h"
#include "cmdlist.h"
#include "session.h"
#include "encoding.h"
#include "bigfile.h"
#include "yenc.h"
#include "uuencode.h"
#include "bitflag.h"
#include "stopwatch.h"

#include "ui/file.h"
#include "ui/download.h"
#include "ui/settings.h"
#include "ui/error.h"

namespace newsflash
{

class download::file 
{
public:
    file(bigfile&& big) : big_(std::move(big)), discard_(false)
    {}

   ~file()
    {
        big_.close();

        if (discard_)
            bigfile::erase(big_.name());
    }

    void write(const std::vector<char>& buff, std::size_t offset)
    {
        if (discard_)
            return;

        if (offset)
            big_.seek(offset);

        big_.write(&buff[0], buff.size());
    }
    void discard_on_close()
    { discard_ = true; }

    std::uint64_t size() const 
    { return big_.size(); }

    std::string name() const
    { return big_.name(); }

private:
    bigfile big_;
private:
    std::atomic<bool> discard_;
};

// decode action decodes a buffer of nntp data
class download::decode : public action
{
public:
    enum class error {
        crc_mismatch, size_mismatch
    };

    class exception : public std::exception
    {
    public:
        exception(std::string what) : what_(std::move(what))
        {}

        const char* what() const noexcept
        { return what_.c_str(); }

    private:
        std::string what_;
    };

    decode(buffer&& article) : article_(std::move(article)), binary_offset_(0), binary_size_(0), encoding_(encoding::unknown)
    {}

    virtual void xperform() override
    {
        // iterate over the content line by line and inspect
        // every line untill we can identify a binary encoding.
        // we're assuming that data before and after binary content
        // is simply textual content such as is often the case with 
        // for example uuencoded images embedded in a text. 
        encoding enc = encoding::unknown;

        nntp::linebuffer lines(article_.content(), article_.content_length());
        nntp::linebuffer::iterator beg = lines.begin();
        nntp::linebuffer::iterator end = lines.end();

        std::size_t binary_start_offset = 0;
        while (beg != end && enc != encoding::unknown)
        {
            const auto line = *beg;
            enc = identify_encoding(line.start, line.length);
            if (enc != encoding::unknown) 
                break;

            binary_start_offset += line.length;            

            // save the text data before the binary in the text buffer
            std::copy(line.start, line.start + line.length, std::back_inserter(text_));
        }

        encoding_ = enc;

        std::size_t consumed = 0;
        const auto dataptr = article_.content() + binary_start_offset;
        const auto datalen = article_.content_length() - binary_start_offset;

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

        if (!text_.empty())
        {
            std::string str = "\r\n[binary content]\r\n";
            std::copy(str.begin(), str.end(), std::back_inserter(text_));
//            std::copy(dataptr + consumed, dataptr + consumed + )
        }

        // store the rest (if any) int the text buffer again
    }

    std::vector<char> get_text_data() 
    { return std::move(text_); }

    std::vector<char> get_binary_data() 
    { return std::move(binary_); }

    std::size_t get_binary_offset() const
    { return binary_offset_; }

    std::size_t get_binary_size() const 
    { return binary_size_; }

    std::string get_binary_name() const 
    { return binary_name_; }

    bitflag<error> get_errors() const 
    { return errors_; }

    encoding get_encoding() const
    { return encoding_; }

private:
    std::size_t decode_yenc_single(const char* data, std::size_t len)
    {
        nntp::bodyiter beg(data);
        nntp::bodyiter end(data + len);

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
        binary_size_   = buff.size();

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
        return std::distance(beg, end);
    }

    std::size_t decode_yenc_multi(const char* data, std::size_t len)
    {
        nntp::bodyiter beg(data);
        nntp::bodyiter end(data, len);

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
        return std::distance(beg, end);
    }

    std::size_t decode_uuencode_single(const char* data, std::size_t len)
    {
        nntp::bodyiter beg(data);
        nntp::bodyiter end(data, len);

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

        return std::distance(beg, end);
    }

    std::size_t decode_uuencode_multi(const char* data, std::size_t len)
    {
        nntp::bodyiter beg(data);
        nntp::bodyiter end(data, len);

        std::vector<char> buff;
        uuencode::decode(beg, end, std::back_inserter(buff));

        // see comments in uuencode_single
        uuencode::parse_end(beg, end);

        binary_        = std::move(buff);
        binary_offset_ = 0; // we have no idea about the offset, uuencode doesn't have it.
        binary_size_   = 0;

        return std::distance(beg, end);
    }
private:
    buffer article_;
private:
    std::vector<char> text_;
    std::vector<char> binary_;    
    std::size_t binary_offset_;    
    std::size_t binary_size_;    
    std::string binary_name_;
private:
    encoding encoding_;
private:
    bitflag<error> errors_;
};

// write action writes a buffer full of binary data to a file
class download::write : public action
{
public:
    write(std::size_t offset, std::vector<char> data, std::shared_ptr<file> f) 
       : offset_(offset), data_(std::move(data)), file_(f)
    {}

    virtual void xperform() override
    {
        stopwatch timer;
        timer.start();

        file_->write(data_, offset_);

        time_ = timer.milliseconds();
    }

    std::size_t bytes_writen() const 
    { return data_.size(); }

    stopwatch::ms_t time_spent() const 
    { return time_; }

private:
    stopwatch::ms_t time_;
private:
    std::size_t offset_;
    std::vector<char> data_;
    std::shared_ptr<file> file_;
};


class download::bodylist : public cmdlist
{
public:
    bodylist(std::deque<std::string> groups, std::deque<std::string> messages) : groups_(std::move(groups)), messages_(std::move(messages)),
        task_(0), account_(0), group_index_(0), group_fail_(false)    
    {}

    virtual bool is_done(cmdlist::step step) const override
    {
        if (step == cmdlist::step::configure)
            return group_fail_ || !(group_index_ < groups_.size());

        return true;
    }

    virtual bool is_good(cmdlist::step step) const override
    { 
        if (step == cmdlist::step::configure)
            return !group_fail_;
        return true;
    }


    virtual void submit(cmdlist::step step, session& sess) override
    {
        if (step == cmdlist::step::configure)
        {
            sess.change_group(groups_[group_index_]);
        }
        else if (step == cmdlist::step::transfer)
        {
            for (const auto& message : messages_)
                sess.retrieve_article(message);
        }
    }

    virtual void receive(cmdlist::step step, buffer& buff) override
    {
        if (step == cmdlist::step::configure)
        {
            if (buff.content_status() == buffer::status::success)
                group_index_ = groups_.size();
        }
        else if (step == cmdlist::step::transfer)
        {
            buffers_.push_back(std::move(buff));
        }
    }
    virtual void next(cmdlist::step step) override
    {
        if (step == cmdlist::step::configure)
        {
            group_fail_ = (++group_index_ == groups_.size());
        }
    }

    virtual std::size_t task() const  override
    { return task_; }

    virtual std::size_t account() const override
    { return account_; }

    void set_task(std::size_t id)
    { task_ = id; }

    void set_account(std::size_t account)
    { account_ = account; }

    const 
    std::deque<std::string>& get_articles()  const
    { return messages_; }

    const
    std::deque<std::string>& get_groups() const
    { return groups_; }

    std::deque<buffer>& get_buffers()
    { return buffers_; }

private:
    const std::deque<std::string> groups_;
    const std::deque<std::string> messages_;
    std::deque<buffer> buffers_;
private:
    std::size_t task_;
    std::size_t account_;
    std::size_t group_index_;
private:
    bool group_fail_;
};

download::download(std::size_t id, std::size_t batch, std::size_t account, const ui::download& details) : 
    id_(id), main_account_(account), num_articles_total_(details.articles.size())
{
    state_.st         = task::state::queued;
    state_.batch      = batch;
    state_.id         = id;
    state_.desc       = details.desc;
    state_.size       = details.size;
    state_.runtime    = 0;
    state_.etatime    = 0;
    state_.completion = 0.0f;

    path_ = fs::remove_illegal_filepath_chars(details.path);
    name_ = fs::remove_illegal_filename_chars(details.desc);
    num_articles_ready_ = 0;
    overwrite_ = false;
    discard_text_ = false;
    started_ = false;

    const auto& groups   = details.groups;
    const auto& articles = details.articles;

    // divide the list of articles into multiple cmdlists

    const std::size_t num_articles_per_cmdlist = 10;
    const std::size_t num_articles = articles.size() + num_articles_per_cmdlist - 1;

    for (std::size_t i=0; i<num_articles/num_articles_per_cmdlist; ++i)
    {
        const auto beg = i * num_articles;
        const auto end = beg + std::min(articles.size() - beg, num_articles_per_cmdlist);
        assert(end <= articles.size());

        std::deque<std::string> next;
        std::copy(articles.begin() + beg, articles.begin() + end, 
            std::back_inserter(next));

        std::unique_ptr<bodylist> cmd(new bodylist(groups, std::move(next)));
        cmd->set_task(id);
        cmd->set_account(account);
        cmds_.push_back(std::move(cmd));
    }

    LOG_D("Task ", id_, " created '", details.desc, "'");
    LOG_D("Task has ", details.articles.size(), " articles for expected size of ", size(details.size));
}

download::~download()
{
    LOG_D("Task ", id_, " deleted");    
}

void download::start()
{
    if (state_.st != state::queued)
        return;

    state_.st = task::state::waiting;
    timer_.start();
    started_ = true;
}


void download::kill()
{
    for (auto& it : files_)
    {
        auto& file = it.second;
        file->discard_on_close();
    }
}

void download::flush()
{}

void download::pause()
{
    if (state_.st == state::paused || 
        state_.st == state::complete || 
        state_.st == state::error)
        return;

    state_.st  = state::paused;
    timer_.pause();
}

void download::resume() 
{
    if (state_.st != state::paused)
        return;

    if (started_)
    {
        if (num_articles_ready_ == num_articles_total_)
        {
            state_.st = task::state::complete;
        }
        else
        {
            state_.st = task::state::waiting;
            timer_.start();
        }
    }
    else 
    {
        state_.st = task::state::queued;
    }
}

bool download::get_next_cmdlist(std::unique_ptr<cmdlist>& cmds)
{
    if (cmds_.empty())
        return false;

    cmds = std::move(cmds_.front());
    cmds_.pop_front();
    return true;
}

void download::complete(std::unique_ptr<action> act)
{
    std::error_code errc;
    std::string what;
    try
    {
        if (act->has_exception())
            act->rethrow();

        auto* p = act.get();

        if (auto* ptr = dynamic_cast<decode*>(p))
            complete(*ptr);
        else if (auto* ptr = dynamic_cast<write*>(p))
            complete(*ptr);

        return;
    }
    catch (const decode::exception& e)
    {
        what = e.what();
    }
    catch (const std::system_error& e)
    {
        what = e.what();
        errc = e.code(); 
    }
    catch (const std::exception& e)
    {
        what = e.what();
    }

    if (on_error)
        on_error({what, state_.desc, errc});

    LOG_E("Task ", state_.id, " error: ", what);
    if (errc != std::error_code())
        LOG_E("Error details: ", errc.value(), ", ", errc.message());

    state_.st  = state::error;
}

void download::complete(std::unique_ptr<cmdlist> cmd)
{
    // the messages that were not available on the primary account
    // we will move them onto the fill account
    auto& cmdlist = dynamic_cast<bodylist&>(*cmd);

    const auto account = cmdlist.account();
    const auto task = cmdlist.task();
    assert(task == id_);

    const auto& groups = cmdlist.get_groups();
    auto& buffers = cmdlist.get_buffers();
    auto articles = cmdlist.get_articles();

    std::deque<std::string> fill_messages;

    for (std::size_t i=0; i<buffers.size(); ++i)
    {
        auto& buffer  = buffers[i];
        const auto& article = articles[0];
        const auto status = buffer.content_status();

        if (status != buffer::status::success)
        {
            if (account != fill_account_)
            {
                if (fill_account_)
                    fill_messages.push_back(article);
            }
            else
            {
                if (status == buffer::status::dmca)
                    state_.errors.set(ui::task::flags::dmca);
                else if (status == buffer::status::unavailable)
                    state_.errors.set(ui::task::flags::unavailable);
                else if (status == buffer::status::error)
                    state_.errors.set(ui::task::flags::error);        
            }
        }
        else
        {
            std::unique_ptr<decode> dec(new decode(std::move(buffer)));
            dec->set_id(id_);
            dec->set_affinity(action::affinity::any_thread);
            on_action(std::move(dec));

            if (state_.st == state::waiting)
                state_.st = state::active;

            ++num_decoding_actions_;
        }
        articles.pop_front();
    }
    // if there are more articles than buffers it means that
    // not all articles were retrieved yet.
    // therefore create a cmdlist of the remaining articles
    if (!articles.empty())
    {
        std::unique_ptr<bodylist> cmds(new bodylist(groups, std::move(articles)));
        cmds->set_task(id_);
        cmds->set_account(main_account_);
        cmds_.push_back(std::move(cmds));
    }

    // fill messages are messages that were not available on the primary
    // account. if we have any then create a new fill message cmdlist
    // using the fill server.
    if (!fill_messages.empty())
    {
        std::unique_ptr<bodylist> cmds(new bodylist(groups, std::move(fill_messages)));
        cmds->set_task(id_);
        cmds->set_account(fill_account_);
        cmds_.push_back(std::move(cmds));
    }
}

void download::configure(const ui::settings& settings)
{
    overwrite_    = settings.overwrite_existing_files;
    discard_text_ = settings.discard_text_content;
    fill_account_ = settings.enable_fill_account ? settings.fill_account : 0;
}

ui::task download::get_ui_state() const
{
    auto ret = state_;
    ret.completion = double(num_articles_ready_) / double(num_articles_total_);        
    ret.runtime    = timer_.seconds();    
    ret.etatime    = 0;

    // todo: update eta, time spent 
    if (state_.st == state::active || 
        state_.st == state::waiting)
    {
        // todo: eta
    }
    return ret;
}

void download::complete(decode& d)
{
    const auto flags  = d.get_errors();    
    const auto size   = d.get_binary_size();
    const auto offset = d.get_binary_offset();    
    const auto data   = d.get_binary_data();    

    auto name = d.get_binary_name();
    if (name.empty())
    {
        if (files_.empty())
            name = name_;
       else name = files_.begin()->first;
   }

   auto it = files_.find(name);
   if (it == std::end(files_))
   {
        bigfile big;

        // try to open a file at the current location multiple times under a different
        // name if overwrite flag is not set.
        for (int i=0; i<10; ++i)
        {
            const auto& next = fs::filename(i, name);
            const auto& file = fs::joinpath(path_, next);
            if (!overwrite_ && bigfile::exists(file))
                continue;

            const auto error = big.create(file);
            if (error != std::error_code())
                throw std::system_error(error, "error creating file: " + file);

            if (size)
            {
                const auto error = bigfile::resize(file, size);
                if (error != std::error_code())
                    throw std::system_error(error, "error resizing file: " + file);
            }
        }
        if (!big.is_open())
            throw std::runtime_error("unable to create files at: " + path_);

        it = files_.insert(std::make_pair(name, std::make_shared<file>(std::move(big)))).first;
    }

    auto& big = it->second;
    std::unique_ptr<write> w(new write(offset, std::move(data), big));
    on_action(std::move(w));

    if (flags.any_bit())
        state_.errors.set(ui::task::flags::damaged);

}

void download::complete(write& w)
{
    --num_decoding_actions_;
    ++num_articles_ready_;
    
    if (num_decoding_actions_ == 0)
    {
        if (state_.st == state::active)
            state_.st = state::waiting;
    }

    if (num_articles_ready_ != num_articles_total_)
        return;

    if (state_.st == state::active)
        state_.st = state::complete;

    for (auto& it : files_)
    {
        const auto& big = it.second;

        ui::file file;
        file.path    = big->name();
        file.size    = big->size();
        file.binary  = true;
        file.damaged = state_.errors.test(ui::task::flags::damaged);

        on_file(file);

        LOG_D("File completed ", file.path);
    }    
}

} // newsflash

