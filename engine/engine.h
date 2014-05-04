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

#pragma once

#include <corelib/connection.h>
#include <corelib/taskstate.h>
#include <corelib/speedometer.h>
#include <corelib/throttle.h>
#include <memory>
#include <mutex>
#include <vector>
#include <deque>
#include <string>
#include <cstdint>
#include "settings.h"

namespace engine
{
    struct file;    
    struct account;
    struct task;
    struct connection;
    class listener;

    // engine manages collections of connections
    // and download tasks/items. 
    class engine
    {
        struct task_t;
        struct conn_t;
        struct batch_t;        

    public:

        // list provides read-only access to the current objects
        // managed by the engine. (tasks, connections and batches)
        template<typename InternalType, typename VisibleType>
        class list
        {
        public:
            typedef std::deque<std::unique_ptr<InternalType>> list_type;

            list(list_type* list) : list_(list)
            {}
            std::size_t size() const
            {
                return list_->size();
            }

            const VisibleType& operator[](std::size_t i) const
            {
                assert(i < size());

                const auto& private_item = (*list_)[i];
                const auto& visible_item = get_visible_state(private_item);
                return visible_item;
            }

        private:
            list_type* list_;
        };

        typedef list<task_t, ::engine::task> tasklist;
        typedef list<conn_t, ::engine::connection> connlist;

        struct stats {
            std::uint64_t bytes_downloaded;
            std::uint64_t bytes_written;
            std::uint64_t bytes_queued;
        };

        // create a new engine with the given listener object and logs folder.
        engine(listener& callback, std::string logs);
       ~engine();

        // set the engine settings.
        void set(const ::engine::settings& settings);

        // set an account in the engine.
        // if the account already exists then the account is updated, otherwise add account.
        void set(const ::engine::account& account);


        tasklist tasks() const;

        connlist conns() const;

        // request the engine to start shutdown. when the engine is ready is shutting
        // down a callback is invoked. Then the engine can be deleted.
        // This 2 phase stop allows the GUI to for example animate while 
        // the engine performs an orderly shutdown
        void shutdown();

        // download a single file
        void download(const ::engine::file& file);

        // download a batch of files
        void download(const std::vector<::engine::file>& files);

        void pump();

    private:
        void on_task_action(task_t* task, corelib::taskstate::action action);

        void on_conn_ready(conn_t* conn);
        void on_conn_error(conn_t* conn, corelib::connection::error error);
        void on_conn_read(conn_t* conn, std::size_t bytes);
        void on_conn_auth(conn_t* conn, std::string& user, std::string& pass);
        bool on_conn_throttle(conn_t* conn, std::size_t& quota);

    private:
        void schedule_tasklist();
        void schedule_connlist();

    private:
        std::string logs_;
        std::mutex mutex_;
        std::deque<std::unique_ptr<task_t>> tasks_;
        std::deque<std::unique_ptr<conn_t>> conns_;
        std::vector<account> accounts_;
        std::uint64_t bytes_downloaded_;
        std::uint64_t bytes_written_;
        std::uint64_t bytes_queued_;
        corelib::throttle throttle_;
        corelib::speedometer meter_;
        settings settings_;
    };

} // engine
