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

#include "download.h"
#include "buffer.h"
#include "filesys.h"
#include "logging.h"
#include "action.h"
#include "session.h"
#include "bigfile.h"
#include "bitflag.h"
#include "stopwatch.h"
#include "decode.h"
#include "bodylist.h"
#include "format.h"
#include "settings.h"

#include "ui/file.h"
#include "ui/download.h"
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


download::download(std::size_t id, std::size_t batch, std::size_t account, const ui::download& details) : stm_(details.articles.size())
{
    task_id_      = id;
    batch_id_     = batch;
    main_account_ = account;
    fill_account_ = 0;
    path_         = details.path;
    desc_         = details.desc;
    overwrite_    = false;
    discard_text_ = false;

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

        std::vector<std::string> next;
        std::copy(articles.begin() + beg, articles.begin() + end, 
            std::back_inserter(next));

        std::unique_ptr<bodylist> cmd(new bodylist(id, account, groups, std::move(next)));
        cmds_.push_back(std::move(cmd));
    }

    //LOG_D("Task ", id_, " created '", details.desc, "'");
    //LOG_D("Task has ", details.articles.size(), " articles for expected size of ", size(details.size));

    stm_.on_stop = [&] {
        timer_.pause();
    };

    stm_.on_cancel = [&] {
        // the task is canceled, rollback changes done to the file system, i.e. any files 
        // that we might have created.
        // note that there might be pending write actions on the files but we simply reduce
        // them to no-ops by setting the discard flag on true and then when the last
        // shared ref to the file goes away and discard is on, the file is deleted.
        for (auto& it : files_)
        {
            auto& file = it.second;
            file->discard_on_close();
        }
    };

    stm_.on_complete = [&] {
        // notify the listeners of any files that we have decoded 
        for (auto& it : files_)
        {
            const auto& big = it.second;

            ui::file file;
            file.path    = big->name();
            file.size    = big->size();
            file.binary  = true;
            file.damaged = errors_.test(ui::task::flags::damaged);
            on_file(file);

            LOG_D("File completed ", file.path);
        }            
    };

    stm_.on_activate = [&] {
        timer_.start();
    };

    stm_.on_state_change = [&] (ui::task::state old, ui::task::state now) {
        LOG_D("Task ", task_id_, old, " => ", now);
    };
}

download::~download()
{
    LOG_D("Task ", task_id_, " deleted");    
}

void download::start()
{
    stm_.start();
}

void download::kill()
{
    stm_.kill();
}

void download::flush()
{}

void download::pause()
{
    stm_.pause();
}

void download::resume() 
{
    stm_.resume();
}

bool download::get_next_cmdlist(std::unique_ptr<cmdlist>& cmds)
{
    if (cmds_.empty())
        return false;

    cmds = std::move(cmds_.front());
    cmds_.pop_front();
    stm_.activate();
    return true;
}

void download::complete(std::unique_ptr<action> act)
{
    std::error_code errc;
    std::string what;
    try
    {
        stm_.deactivate();

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
        on_error({what, desc_, errc});

    //LOG_E("Task ", state_.id, " error: ", what);
    if (errc != std::error_code())
        LOG_E("Error details: ", errc.value(), ", ", errc.message());

    stm_.error();
}

void download::complete(std::unique_ptr<cmdlist> cmd)
{
    stm_.deactivate();

    // 1. all succesfully downloaded buffers are sent for decoding
    // 2. those that have not been succesfully downloaded yet are rescheduled
    // those that were incomplete or not unavaiable or rescheduled for the fill account if it is enabled

    auto list = std::unique_ptr<bodylist>((bodylist*)cmd.release());

    const auto account = list->account();

    if (list->configure_fail())
    {
        if (fill_account_ && (account != fill_account_))
        {
            list->set_account(fill_account_);
            cmds_.push_back(std::move(list));
        }
        else
        {
            errors_.set(ui::task::flags::unavailable);
            stm_.complete_articles(list->num_messages());
        }
        return;
    }

    const auto& messages = list->get_messages();
    const auto& groups   = list->get_groups();

    auto buffers = list->get_buffers();

    std::vector<std::string> fillers;

    for (std::size_t i=0; i<buffers.size(); ++i)
    {
        auto& buf = buffers[i];
        const auto& msg = messages[i];
        const auto status = buf.content_status();
        if (status != buffer::status::success)
        {
            if (fill_account_ && (account != fill_account_))
            {
                fillers.push_back(msg);
            }
            else
            {
                if (status == buffer::status::dmca)
                    errors_.set(ui::task::flags::dmca);
                else if (status == buffer::status::unavailable)
                    errors_.set(ui::task::flags::unavailable);
                else if (status == buffer::status::error)
                    errors_.set(ui::task::flags::error);

                stm_.complete_articles(1);
            }
        }
        else
        {
            std::unique_ptr<decode> dec(new decode(std::move(buf)));
            dec->set_id(task_id_);
            dec->set_affinity(action::affinity::any_thread);
            on_action(std::move(dec));
            stm_.activate();
        }
    }
    
    if (!list->is_complete())
    {
        cmds_.push_back(std::move(list));
    }

    if (!fillers.empty())
    {
        std::unique_ptr<bodylist> cmd(new bodylist(task_id_, fill_account_, groups, fillers));
        cmds_.push_back(std::move(cmd));
    }
}

void download::configure(const settings& s)
{
    overwrite_    = s.overwrite_existing_files;
    discard_text_ = s.discard_text_content;
    //fill_account_ = s.enable_fill_account ? s.fill_account : 0;
}

ui::task download::get_ui_state() const
{
    ui::task state;
    state.batch   = batch_id_;
    state.id      = task_id_;
    state.errors  = errors_;
    state.desc    = desc_;
    state.size    = expected_size_;
    state.runtime = timer_.seconds();

    return state;

    // auto ret = state_;
    // //ret.completion = double(num_articles_ready_) / double(num_articles_total_);        
    // ret.runtime    = timer_.seconds();    
    // ret.etatime    = 0;
    // ret.st         = stm_.get_state();

    // // todo: update eta, time spent 
    // if (state_.st == state::active || state_.st == state::waiting)
    // {
    //     // todo: eta
    // }
    // return ret;
}

void download::complete(decode& d)
{
    const auto flags  = d.get_errors();    
    const auto size   = d.get_binary_size();
    const auto offset = d.get_binary_offset();    
    const auto data   = d.get_binary_data();    

    if (data.empty())
    {
        stm_.deactivate();        
        stm_.complete_articles(1);
        return;
    }

    auto name = d.get_binary_name();
    if (name.empty())
    {
        if (files_.empty())
            name = desc_;
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
    stm_.activate();

    if (flags.any_bit())
        errors_.set(ui::task::flags::damaged);

}

void download::complete(write& w)
{
    stm_.deactivate();    
    stm_.complete_articles(1);
}

} // newsflash

