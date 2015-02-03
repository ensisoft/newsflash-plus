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

namespace newsflash
{
    // cmdlist encapsulates a sequence of nntp operations to be performed
    // for example downloading articles or header overview data.
    class cmdlist
    {
    public:
        enum class type {
            xover, body, list
        };

        using list = std::vector<std::string>;

        cmdlist() : cancelbit_(false), failbit_(false), account_(0), task_(0), conn_(0),
            cmdtype_(type::list)
        {}

        cmdlist(list groups, list commands, cmdlist::type type) : cancelbit_(false), failbit_(false), account_(0), task_(0), conn_(0),
            groups_(std::move(groups)), commands_(std::move(commands)), cmdtype_(type)
        {}
        // the cmdlist operation is divided into 2 steps, session configuration
        // and then the actual data transfer. 
        // session configuration is performed step-by-step (without pipelining)
        // where as the data transfer can be pipelined. 

       ~cmdlist()
        {}

        bool needs_to_configure() const 
        {
            return cmdtype_ != type::list;
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
            if (cmdtype_ == type::list)
            {
                ses.retrieve_list();
                return;
            }

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

        void cancel()
        { cancelbit_ = true; }
        
        bool is_canceled() const
        { return cancelbit_;}

        bool is_good() const 
        { return !failbit_; }

        void set_account(std::size_t aid)
        { account_ = aid; }

        void set_task(std::size_t tid)
        { task_ = tid; }

        void set_conn(std::size_t cid)
        { conn_ = cid; }


    protected:
        std::atomic<bool> cancelbit_;
        std::atomic<bool> failbit_;
        std::size_t account_;
        std::size_t task_;
        std::size_t conn_;
        std::vector<buffer> buffers_;
    private:
        list groups_;
        list commands_;
    private:
        type cmdtype_;
    };

} // newsflash
