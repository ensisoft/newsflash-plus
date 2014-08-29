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
#include "engine.h"
#include "connection.h"
#include "task.h"
#include "action.h"
#include "assert.h"
#include "filesys.h"
#include "logging.h"

namespace newsflash
{

class engine::batch : public task
{
public:
};

engine::engine(std::string logdir) : threads_(4)
{
    LOG_OPEN("engine", fs::joinpath(logdir, "engine.log"));

    threads_.on_complete = std::bind(&engine::on_action_complete, this,
        std::placeholders::_1);

    settings_.auto_remove_completed    = false;
    settings_.discard_text_content     = false;
    settings_.enable_fill_account      = false;
    settings_.overwrite_existing_files = false;
    settings_.prefer_secure            = true;
    settings_.throttle                 =  0;
    settings_.fill_account = 0;
}

engine::~engine()
{
    threads_.wait();
}

void engine::download(const ui::account& account, const ui::file& file)
{
    download(account, {file});
}

void engine::download(const ui::account& account, const std::vector<ui::file>& files)
{
    auto it = std::find_if(std::begin(accounts_), std::end(accounts_), 
        [&](const ui::account& acc) {
            return acc.id == account.id;
        });
    if (it == std::end(accounts_))
        accounts_.push_back(account);
    else 
        *it = account;

    if (settings_.auto_connect)
        start();
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
            complete_action(act, it->get());
        }
        else
        {
            auto it = std::find_if(std::begin(tasks_), std::end(tasks_),
                [&](const std::unique_ptr<task>& task) {
                    return task->get_id() == id;
                });
            ASSERT(it != std::end(tasks_));
            complete_action(act, it->get());
        }
    }
    if (settings_.auto_remove_completed)
        remove_completed_tasks();
}

void engine::configure(const ui::settings& settings)
{
    settings_ = settings;
    if (settings_.auto_connect)
        start();
    else stop();
}

void engine::on_action_complete(action* act)
{
    std::unique_lock<std::mutex> lock(mutex_);
    actions_.push(act);
    lock.unlock();

    on_async_notify();
}

void engine::start()
{
    // find a next runnable task
    task* runnable = nullptr;

    using state = task::state;

    for (auto& t : tasks_)
    {
        const auto st = t->get_state();
        if (st == state::queued || st == state::waiting || st == state::active)
        {

        }
    }

}

void engine::stop()
{}

void engine::complete_action(action* act, connection* conn)
{
    std::unique_ptr<action> a(act);

    conn->complete(std::move(a));

    const auto state = conn->get_state();

    if (state == connection::state::connected)
    {

    }
    else if (state == connection::state::error)
    {

    }
    else if (state == connection::state::disconnected)
    {}

}

void engine::complete_action(action* act, task* t)
{
    LOG_SELECT("engine");

    std::unique_ptr<action> a(act);

    try
    {
        t->complete(std::move(a));
    }
    catch (const std::exception& e)
    {}

    if (settings_.auto_connect)
        start();
}

void engine::remove_completed_tasks()
{

}

} // newsflash
