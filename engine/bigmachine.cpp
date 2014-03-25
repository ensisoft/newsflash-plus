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

#include <corelib/connection.h>
#include <corelib/taskstate.h>
#include <corelib/threadpool.h>
#include <corelib/download.h>
#include <corelib/logging.h>
#include <corelib/assert.h>
#include <corelib/filesys.h>
#include <corelib/waithandle.h>
#include <corelib/speedometer.h>
#include <corelib/throttle.h>
#include <corelib/format.h>
#include <corelib/etacalc.h>
#include <algorithm>
#include <functional>
#include <vector>
#include <thread>
#include "configuration.h"
#include "bigmachine.h"
#include "connection.h"
#include "listener.h"
#include "account.h"
#include "server.h"
#include "task.h"

namespace engine
{

class bigmachine::impl
{
public:
    impl(listener& callback, std::string logs) : logs_(std::move(logs)), listener_(callback)
    {


        thread_.reset(new std::thread(std::bind(&bigmachine::impl::thread_main, this)));
    }

   ~impl()
    {}

    void configure(const configuration& conf)
    {
        enqueue(std::bind(&impl::queued_configure, this, conf));
    }
    void shutdown()
    {
    }

    void configure_account(const account& acc)
    {
        enqueue(std::bind(&impl::queued_add_account, this, acc));
    }

    void download(const bigmachine::content& cont)
    {

    }

    void download(const std::vector<bigmachine::content>& contents)
    {


    }
private:
    struct task_t {
        std::size_t id;
        std::uint64_t size;
        std::string description;
        task_error error;
        task_state state;        
        corelib::taskstate stm;
        corelib::etacalc eta;
    };

    struct batch_t {

    };

    struct conn_t {
        std::size_t id;
        std::size_t account;
        std::size_t task;
        std::uint64_t bytes;
        std::string host;
        connection_error error;        
        connection_state state;
        corelib::connection conn;        
        corelib::speedometer speedometer;
        bool secure;

        conn_t(const std::string& logfile) : conn(logfile)
        {}
    };

private:
    // remember, all the connection callbacks are executed by the connection thread.
    // the event either needs to be marshalled to the engine thread or then locking must be done!
    void on_connection_ready(conn_t* conn)
    {
        enqueue(std::bind(&impl::queued_connection_ready, this, conn));
    }

    void on_connection_error(corelib::connection::error err, conn_t* conn)
    {
        enqueue(std::bind(&impl::queued_connection_error, this, err, conn));
    }

    void on_connection_read(std::size_t bytes, conn_t* conn)
    {
        enqueue(std::bind(&impl::queued_connection_read, this, bytes, conn));
    }

    void on_connection_auth(std::string& user, std::string& pass, conn_t* conn)
    {
        // lookup the username and password from the account
        // that this connection belongs to.
        // the account must be available.
        std::lock_guard<std::mutex> lock(mutex_);

        const auto it = std::find_if(accounts_.begin(), accounts_.end(),
            [=](const account& maybe) {
                return conn->account  == maybe.id;
            });

        ASSERT(it != accounts_.end());

        const auto acc = *it;
        user = acc.username;
        pass = acc.password;
    }

    void on_connection_throttle(conn_t* conn)
    {
        // todo:
        std::lock_guard<std::mutex> lock(mutex_);
    }

private:
    void spawn_connections(const account& acc)
    {
        auto cons = std::count_if(connections_.begin(), connections_.end(),
            [=](const std::unique_ptr<conn_t>& conn)
            {
                return conn->account == acc.id;
            });

        bool secure  = false;
        const server* serv = &acc.general;
        if (config_.prefer_secure && is_valid(acc.secure))
        {
            serv   = &acc.secure;
            secure = true;
            LOG_I("Using secure server");
        }

        for (; cons < acc.max_connections; ++cons)
        {
            const auto cid = connections_.size() + 1;
            const auto log = corelib::format("connection", cid, ".log");

            LOG_I("Spawning a new connection to: ", serv->host, ":", serv->port);            
            LOG_I("Connection id ", cid);
            LOG_I("Logfile '", log, "'");

            std::unique_ptr<conn_t> conn(new conn_t(log));

            conn->id      = cid;
            conn->account = acc.id;
            conn->error   = connection_error::none;
            conn->state   = connection_state::connecting;
            conn->secure  = secure;
            conn->host    = serv->host;

            conn->conn.on_ready = std::bind(&impl::on_connection_ready, this, conn.get());
            conn->conn.on_error = std::bind(&impl::on_connection_error, this, 
                std::placeholders::_1, conn.get());
            conn->conn.on_read  = std::bind(&impl::on_connection_read, this, 
                std::placeholders::_1, conn.get());
            conn->conn.on_auth  = std::bind(&impl::on_connection_auth, this,
                std::placeholders::_1, std::placeholders::_2, conn.get());

            conn->conn.connect(serv->host, serv->port, secure);
            connections_.push_back(std::move(conn));

            LOG_I("Connection started....");
        }
    }

    void schedule_connections()
    {

    }


    void schedule_tasks()
    {
        for (const auto& task : tasks_)
        {
        }
    }

    void thread_main()
    {
        LOG_OPEN(fs::joinpath(logs_, "engine.log"));
        LOG_D("Engine thread: ", std::this_thread::get_id());

        const std::chrono::milliseconds SCHEDULE_INTERVAL(100);

        bytes_downloaded_ = 0;
        bytes_written_    = 0;
        bytes_queued_     = 0;

        for (;;)
        {
            auto events   = events_.wait();
            auto shutdown = shutdown_.wait();
            if (corelib::wait_for(events, shutdown, SCHEDULE_INTERVAL))
            {
                if (shutdown)
                    break;

                event next_event;
                dequeue(next_event);
                next_event();
            }
            else
            {
             //   spawn_connections();
              //  schedule_connections();
              //  schedule_tasks();
            }
        }

        LOG_D("Engine thread exiting...");
        LOG_CLOSE();
    }

    void queued_connection_ready(conn_t* conn)
    {
        LOG_I("Connection ", conn->id, " ready");

        conn->state = connection_state::ready;
        conn->error = connection_error::none;


    }

    void queued_connection_error(corelib::connection::error error, conn_t* conn)
    {
        conn->state = connection_state::error;

        LOG_E("Connection ", conn->id, " error");
        switch (error)
        {
            case corelib::connection::error::none:
                assert(0);
                break;

            case corelib::connection::error::resolve:
                LOG_E("Failed to resolve host ", conn->host);
                conn->error = connection_error::resolve;
                break;

            case corelib::connection::error::refused:
                LOG_E("Connection refused");
                conn->error = connection_error::refused;
                break;

            case corelib::connection::error::forbidden:
                LOG_E("Authentication failed");
                return;

            case corelib::connection::error::protocol:
            case corelib::connection::error::interrupted:
            case corelib::connection::error::unknown:
                break;

            case corelib::connection::error::socket:
            case corelib::connection::error::ssl:
                LOG_E("Network error");

                break;

            case corelib::connection::error::timeout:
                LOG_E("Network timeout");
                break;
        }
        //conn->

    }

    void queued_connection_read(std::size_t bytes, conn_t* conn)
    {
        if (!speedometer_.is_running())
            speedometer_.start();

        if (!conn->speedometer.is_running())
            conn->speedometer.start();

        speedometer_.submit(bytes);
        bytes_downloaded_ += bytes;

        conn->speedometer.submit(bytes);
        conn->bytes += bytes;
    }

    void queued_add_account(const account& acc)
    {
        auto it = std::find_if(accounts_.begin(), accounts_.end(),
            [=](const account& maybe)
            {
                if (maybe.id == acc.id)
                    return true;
                return false;
            });
        if (it == accounts_.end())
            accounts_.push_back(acc);
        else *it = acc;
    }

    void queued_configure(const configuration& config)
    {
        config_ = config;
    }

private:
    typedef std::function<void(void)> event;

private:
    std::size_t next_id()
    {
        return id_++;
    }

    void enqueue(const event& ev)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        eventqueue_.push(ev);
        if (eventqueue_.size() == 1)
            events_.set();
    }
    void dequeue(event& ev)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        ev = eventqueue_.front();
        eventqueue_.pop();
        if (eventqueue_.empty())
            events_.reset();
    }

private:
    const std::string logs_;

private:
    std::condition_variable cond_;
    std::mutex mutex_;
    std::queue<event> eventqueue_;
    std::vector<account> accounts_;
    std::deque<std::unique_ptr<conn_t>> connections_;
    std::deque<std::unique_ptr<task_t>> tasks_;
    std::deque<batch_t*> batches_;
    std::unique_ptr<std::thread> thread_;
    std::uint64_t bytes_downloaded_;
    std::uint64_t bytes_written_;
    std::uint64_t bytes_queued_;
    std::size_t   id_;
    corelib::event events_;
    corelib::event shutdown_;
    corelib::event schedule_;
    corelib::speedometer speedometer_;
    corelib::throttle throttle_;
    listener& listener_;
    configuration config_;
    bool run_;
};

bigmachine::bigmachine(listener& callback, std::string logs) : pimpl_(new impl(callback, std::move(logs)))
{
    fs::create_path(logs);
}

void bigmachine::configure(const configuration& conf)
{
    pimpl_->configure(conf);
}

void bigmachine::configure(const account& acc)
{
    pimpl_->configure_account(acc);
}

void bigmachine::download(const bigmachine::content& cont)
{
    pimpl_->download(cont);
}

void bigmachine::download(const std::vector<bigmachine::content>& contents)
{
    pimpl_->download(contents);
}

} // bigmachine
