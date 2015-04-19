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

#include <newsflash/config.h>
#include <cstddef>
#include <atomic>
#include <vector>
#include <string>
#include "session.h"
#include "buffer.h"
#include "assert.h"

namespace newsflash
{
    // cmdlist encapsulates a sequence of nntp operations to be performed
    // for example downloading articles or header overview data.
    // the cmdlist operation is divided into 2 steps, session configuration
    // and then the actual data transfer. 
    // session configuration is performed step-by-step (without pipelining)
    // where as the data transfer can be pipelined.     
    class cmdlist
    {
    public:
        enum class type {
            xover, body, listing, groupinfo
        };

        struct listing {};
        struct messages {
            std::vector<std::string> groups;  // groups in which to look for the messages
            std::vector<std::string> numbers; // either article numbers or IDs
        };
        struct overviews {
            std::string group;
            std::vector<std::string> ranges;
        };
        struct groupinfo {
            std::string group;
        };

        cmdlist(listing) : cmdlist()
        {
            cmdtype_ = type::listing;
        }
        cmdlist(messages m) : cmdlist()
        {
            groups_   = std::move(m.groups);
            commands_ = std::move(m.numbers);
            cmdtype_  = type::body;
        }
        cmdlist(overviews o) : cmdlist()
        {
            groups_.push_back(std::move(o.group));
            commands_  = std::move(o.ranges);
            cmdtype_   = type::xover;
        }
        cmdlist(groupinfo g) : cmdlist()
        {
            groups_.push_back(std::move(g.group));
            cmdtype_ = type::groupinfo;
        }
       ~cmdlist()
        {}

        bool needs_to_configure() const 
        {
            if (cmdtype_ == type::xover || cmdtype_ == type::body)
                return true;

            return false;
        }

        // try the ith step to configure the session state
        bool submit_configure_command(std::size_t i, session& ses)
        {
            if (i == groups_.size())
                return false;

            // try the ith group in the list of newsgroups that are supposed
            // to carry the articles. we stop after being able to succesfully
            // select whatever group comes first.
            ses.change_group(groups_[i]);
            return true;
        }

        // receive and handle ith response to ith configure command.
        bool receive_configure_buffer(std::size_t i, const buffer& buff)
        {
            // if the group exists, it's good enough for us.
            if (buff.content_status() == buffer::status::success)
                return true;

            // if we have no more groups to check, we're done.
            if (i == groups_.size() -1)
                failbit_ = true;

            return false;
        }

        // submit the data transfer commands as a single batch.
        void submit_data_commands(session& ses) 
        {
            if (cmdtype_ == type::listing)
            {
                ses.retrieve_list();
            }
            else if (cmdtype_ == type::groupinfo)
            {
                ASSERT(!groups_.empty());
                ses.change_group(groups_[0]);
            }
            else
            {
                for (std::size_t i=0; i<commands_.size(); ++i)
                {
                    if (i < buffers_.size() &&
                        buffers_[i].content_status() == buffer::status::success)
                        continue;

                    if (cmdtype_ == type::body)
                        ses.retrieve_article(commands_[i]);
                    else if (cmdtype_ == type::xover)
                        ses.retrieve_headers(commands_[i]);

                    if (i < buffers_.size())
                        buffers_[i].clear();
                }
            }
        }

        // receive a data buffer response to submit_data_commands
        void receive_data_buffer(buffer buff)
        {
            for (auto& old : buffers_)
            {
                if (old.content_status() == buffer::status::none)
                {
                    old = std::move(buff);
                    return;
                }
            }
            buffers_.push_back(std::move(buff));
        }

        std::size_t num_data_commands() const
        {
            return commands_.size();
        }

        std::vector<buffer>& get_buffers() 
        { return buffers_; }

        std::vector<std::string>& get_commands()
        { return commands_; }

        std::size_t account() const 
        { return account_; }

        std::size_t task() const 
        { return task_; }

        std::size_t conn() const 
        { return conn_; }

        std::size_t id() const 
        { return id_; }

        type cmdtype() const 
        { return cmdtype_; }

        // cancel the cmdlist. the cmdlist is to be discarded.
        // i.e. the task has been cancelled.
        void cancel()
        { cancelbit_ = true; }
        
        bool is_canceled() const
        { return cancelbit_;}

        bool is_good() const 
        { return !failbit_; }

        bool is_empty() const 
        { return buffers_.empty(); }

        void set_account(std::size_t aid)
        { account_ = aid; }

        void set_task(std::size_t tid)
        { task_ = tid; }

        void set_conn(std::size_t cid)
        { conn_ = cid; }


    private:
        cmdlist() : cancelbit_(false), failbit_(false), account_(0), task_(0), conn_(0)
        {
            static std::atomic<std::size_t> id(1);
            id_ = id++;
        }

    private:
        type cmdtype_;
        
    private:
        std::atomic<bool> cancelbit_;
        std::atomic<bool> failbit_;
        std::size_t account_;
        std::size_t task_;
        std::size_t conn_;
        std::size_t id_;
        std::vector<buffer> buffers_;
        std::vector<std::string> groups_;
        std::vector<std::string> commands_;
    };

} // newsflash
