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

// #include <corelib/connection.h>
// #include <corelib/speedometer.h>
// #include <corelib/throttle.h>
// #include <corelib/bodylist.h>
// #include <corelib/xoverlist.h>
// #include <msglib/queue.h>
// #include <msglib/message.h>
// #include <memory>
// #include <mutex>
// #include <vector>
// #include <deque>
// #include <string>
// #include <queue>
// #include <cstdint>
// #include "task_state_machine.h"
// #include "settings.h"

// namespace newsflash
// {
//     struct file;    
//     struct account;
//     struct task;
//     struct connection;
//     class listener;

//     // engine manages collections of connections
//     // and download tasks/items. 
//     class engine
//     {
//         struct task_t;
//         struct conn_t;
//         struct batch_t;        

//     public:

//         // list provides read-only access to the current objects
//         // managed by the engine. (tasks, connections and batches)
//         template<typename InternalType, typename VisibleType>
//         class list
//         {
//         public:
//             typedef std::deque<std::unique_ptr<InternalType>> list_type;

//             list(list_type* list) : list_(list)
//             {}
//             std::size_t size() const
//             {
//                 return list_->size();
//             }

//             const VisibleType& operator[](std::size_t i) const
//             {
//                 assert(i < size());

//                 const auto& private_item = (*list_)[i];
//                 const auto& visible_item = get_visible_state(private_item);
//                 return visible_item;
//             }

//         private:
//             list_type* list_;
//         };

//         typedef list<task_t, newsflash::task> tasklist;
//         typedef list<conn_t, newsflash::connection> connlist;
//         typedef list<batch_t, newsflash::task> batchlist;

//         struct stats {
//             std::uint64_t bytes_downloaded;
//             std::uint64_t bytes_written;
//             std::uint64_t bytes_queued;
//         };

//         // create a new engine with the given listener object and logs folder.
//         engine(listener& callback, std::string logs);
//        ~engine();

//         // set the engine settings.
//         void set(const newsflash::settings& settings);

//         // set an account in the engine.
//         // if the account already exists then the account is updated, otherwise add account.
//         void set(const newsflash::account& account);


//         tasklist tasks() const;

//         connlist conns() const;



//         // download a single file
//         void download(const newsflash::account& account, const newsflash::file& file);

//         // download a batch of files
//         void download(const newsflash::account& account, const std::vector<newsflash::file>& files);

//         // process pending messages coming from asynchronous worker threads.
//         // this function should be called as a response to listener::on_events()
//         void pump();

//         // master switch. when stopped all processing is halted
//         // and connections are killed.
//         void stop();

//         void start();

//         bool is_running() const;


//         // request the engine to initiate permanent shutdown. when the engine is ready is shutting
//         // down a callback is invoked. Then the engine can be deleted.
//         // This 2 phase stop allows the GUI to for example animate while 
//         // the engine performs an orderly shutdown
//         //void shutdown();        

//     private:
//         void on_task_start(task_t* task, batch_t* batch);
//         void on_task_stop(task_t* task, batch_t* batch);
//         void on_task_action(task_t* task, batch_t* batch, task::action action);
//         void on_task_state(task_t* task, batch_t* batch, task::state current, task::state next);
//         void on_task_body(task_t* task, batch_t* batch, corelib::bodylist::body&& body);
//         void on_task_xover(task_t* task, batch_t* batch, corelib::xoverlist::xover&& xover);
//         void on_task_xover_prepare(task_t* task, batch_t* batch, std::size_t range_count);

//         void on_conn_ready(conn_t* conn);
//         void on_conn_error(conn_t* conn, corelib::connection::error error, const std::error_code& system_error);
//         void on_conn_read(conn_t* conn, std::size_t bytes);
//         void on_conn_auth(conn_t* conn, std::string& user, std::string& pass);

//     private:
//         void start_next_task(std::size_t account);
//         void start_connections(std::size_t account);

//     private:
//         newsflash::account& find_account(std::size_t id);

//     private:
//         typedef std::function<void (void) > message;

//     private:
//         struct process_buffer;

//     private:
//         std::string logs_;
//         std::mutex mutex_;
//         std::deque<std::unique_ptr<task_t>> tasks_;
//         std::deque<std::unique_ptr<conn_t>> conns_;
//         std::deque<std::unique_ptr<batch_t>> batches_;
//         std::vector<account> accounts_;
//         std::uint64_t bytes_downloaded_;
//         std::uint64_t bytes_written_;
//         std::uint64_t bytes_queued_;
//         double bps_;
//         corelib::throttle throttle_;
//         corelib::speedometer meter_;
//         msglib::queue<message> messages_;
//         settings settings_;
//         listener& listener_;
//         bool stop_;
//     };

// } // engine
