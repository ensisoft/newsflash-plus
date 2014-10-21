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

#include <algorithm>
#include <cassert>
#include "engine.h"
#include "connection.h"
#include "download.h"
#include "action.h"
#include "assert.h"
#include "filesys.h"
#include "logging.h"
#include "utf8.h"

namespace newsflash
{

engine::engine() : threads_(4)
{
    threads_.on_complete = std::bind(&engine::on_action_complete, this,
        std::placeholders::_1);

    flags_.set(flags::prefer_secure);
}

engine::~engine()
{
//    threads_.wait();
}

void engine::set(const account& acc)
{
    auto it = std::find_if(std::begin(accounts_), std::end(accounts_),
        [&](const account& a) {
            return a.id == acc.id;
        });
    if (it == std::end(accounts_))
    {
        accounts_.push_back(acc);
        return;
    }

    const auto& old = *it;

    const bool general_host_updated = (old.general_host != acc.general_host) ||
        (old.general_port != acc.general_port);
    const bool secure_host_updated = (old.secure_host != acc.secure_host) ||
        (old.secure_port != acc.secure_port);
    const bool credentials_updated = (old.username != acc.username) ||
        (old.password != acc.password);

    for (auto it = conns_.begin(); it != conns_.end(); ++it)
    {
        auto& conn = *it;
        if (conn->get_account() != acc.id)
            continue;

        bool discard = false;

        if (general_host_updated)
        {
            if (conn->hostname() == old.general_host ||
                conn->hostport() == old.general_port)
                discard = true;
        }
        if (secure_host_updated)
        {
            if (conn->hostname() == old.secure_host ||
                conn->hostport() == old.secure_port)
                discard = true;
        }
        if (credentials_updated)
            discard = true;

        if (!discard)
            continue;

        conn->disconnect();
        it = conns_.erase(it);
    }
}

void engine::download(std::size_t acc, std::vector<ui::download> batch)
{
    auto it = std::find_if(std::begin(accounts_), std::end(accounts_),
        [=](const account& a) {
            return a.id == acc;
        });
    assert(it != std::end(accounts_));

    for (auto& spec : batch)
    {
        const std::size_t tid = tasks_.size();
        const std::size_t bid = 1234;
        std::unique_ptr<task> job(new class download(tid, bid, acc, spec));

        LOG_D("New download: (", tid, "), '", spec.desc, ")");

        tasks_.push_back(std::move(job));
    }

    if (started_)
        begin_next_task();
}

void engine::pump()
{
    for (;;)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (actions_.empty())
            return;

        auto* act = actions_.front();
        actions_.pop();
        lock.unlock();

        const auto id = act->get_id();
        if (id & 0xFF)
        {
            auto it = std::find_if(std::begin(conns_), std::end(conns_),
                [&](const std::unique_ptr<connection>& conn) {
                    return conn->get_id() == id;
                });
            ASSERT(it != std::end(conns_));
            complete_conn_action(act, it->get());
        }
        else
        {
            auto it = std::find_if(std::begin(tasks_), std::end(tasks_),
                [&](const std::unique_ptr<task>& task) {
                    return task->get_id() == id;
                });
            ASSERT(it != std::end(tasks_));
            complete_task_action(act, it->get());
        }
    }
    if (flags_.test(flags::auto_remove_complete))
        remove_completed_tasks();
}

void engine::start()
{
    if (started_)
        return;

    if (!log_.is_open())
    {
        const auto& file = fs::joinpath(logpath_, "engine.log");
        log_.open(file, std::ios::trunc);
        if (!log_.is_open())
        {
            ui::error err;
            err.what = "failed to open logfile";
            err.resource = file;
            //err.code = std::err
            on_error(err);
        }
    }

    LOG_I("Engine starting...");


    started_ = true;
}

void engine::stop()
{
    if (!started_)
        return;

    // ...

    started_ = false;
}


void engine::configure(bitflag<flags> settings)
{
    LOG_D("Ovewrite existing files: ", 
        settings.test(flags::overwrite_existing_files));
    LOG_D("Discard text content: ", 
        settings.test(flags::discard_text_content));
    LOG_D("Auto remove complete",
        settings.test(flags::auto_remove_complete));
    LOG_D("Prefer secure: ",
        settings.test(flags::prefer_secure));

    flags_ = settings;

    //for (auto& t : tasks_)
    //    t->configure(s);
}

void engine::set_log_folder(std::string path)
{
    LOG_D("Logfile path: ", path);
    logpath_ = std::move(path);
}

void engine::set_throttle(bool on_off, unsigned value)
{
    LOG_D("Throttle is: ", on_off, " value: ", value);
    // todo:
}

void engine::on_action_complete(action* act)
{
    // this callback gets invoked by the thread pool threads
    // it needs to be thread safe.

    // push an action completed on the background into the ready action
    // queue and then notify the client thread.
    std::unique_lock<std::mutex> lock(mutex_);
    actions_.push(act);
    lock.unlock();

    // notify for pump()
    on_async_notify();
}




void engine::complete_conn_action(action* act, connection* conn)
{
}

void engine::complete_task_action(action* act, task* t)
{
}

void engine::remove_completed_tasks()
{
}

void engine::begin_next_task()
{

}

std::ostream* engine::get_log() 
{ return &log_; }

} // newsflash
