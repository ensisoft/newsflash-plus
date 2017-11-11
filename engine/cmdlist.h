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

#pragma once

#include "newsflash/config.h"

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
    class CmdList
    {
    public:
        // the type of data that the cmdlist will get.
        enum class Type {
            // buffer(s) are XOVER header data.
            Header,

            // buffer(s) are BODY article data
            Article,

            // buffer(s) are LIST news group listing data
            Listing,

            // buffer(s) are GROUP information
            GroupInfo
        };

        struct Listing {};

        struct Messages {
            std::vector<std::string> groups;  // groups in which to look for the messages
            std::vector<std::string> numbers; // either article numbers or IDs
        };
        struct Overviews {
            std::string group;
            std::vector<std::string> ranges;
        };
        struct GroupInfo {
            std::string group;
        };

        CmdList(const Listing&) : CmdList()
        {
            cmdtype_ = Type::Listing;
        }
        CmdList(const Messages& m) : CmdList()
        {
            groups_   = m.groups;
            commands_ = m.numbers;
            cmdtype_  = Type::Article;
        }
        CmdList(const Overviews& o) : CmdList()
        {
            groups_.push_back(o.group);
            commands_  = o.ranges;
            cmdtype_   = Type::Header;
        }
        CmdList(const GroupInfo& g) : CmdList()
        {
            groups_.push_back(g.group);
            cmdtype_ = Type::GroupInfo;
        }

        bool NeedsToConfigure() const
        {
            if (cmdtype_ == Type::Header || cmdtype_ == Type::Article)
                return true;

            return false;
        }

        // try the ith step to configure the session state
        bool SubmitConfigureCommand(std::size_t i, session& ses)
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
        bool ReceiveConfigureBuffer(std::size_t i, const buffer& buff)
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
        void SubmitDataCommands(session& ses)
        {
            if (cmdtype_ == Type::Listing)
            {
                ses.retrieve_list();
            }
            else if (cmdtype_ == Type::GroupInfo)
            {
                assert(!groups_.empty());
                ses.retrieve_group_info(groups_[0]);
            }
            else
            {
                for (std::size_t i=0; i<commands_.size(); ++i)
                {
                    if (i < buffers_.size() &&
                        buffers_[i].content_status() == buffer::status::success)
                        continue;

                    if (cmdtype_ == Type::Article)
                        ses.retrieve_article(commands_[i]);
                    else if (cmdtype_ == Type::Header)
                        ses.retrieve_headers(commands_[i]);

                    if (i < buffers_.size())
                        buffers_[i].clear();
                }
            }
        }

        // receive a data buffer response to submit_data_commands
        void ReceiveDataBuffer(const buffer& buff)
        {
            for (auto& old : buffers_)
            {
                if (old.content_status() == buffer::status::none)
                {
                    old = buff;
                    return;
                }
            }
            buffers_.push_back(buff);
        }

        void ReceiveDataBuffer(buffer&& buff)
        {
            for (auto& old : buffers_)
            {
                if (old.content_status() == buffer::status::none)
                {
                    old = std::move(buff);
                    return;
                }
            }
            buffers_.emplace_back(std::move(buff));
        }

        std::size_t NumDataCommands() const
        { return commands_.size(); }

        std::size_t NumBuffers() const
        { return buffers_.size(); }

        std::vector<buffer>& GetBuffers()
        { return buffers_; }

        buffer& GetBuffer(std::size_t i)
        {
            ASSERT(i < buffers_.size());
            return buffers_[i];
        }

        const buffer& GetBuffer(std::size_t i) const
        {
            ASSERT(i < buffers_.size());
            return buffers_[i];
        }

        std::vector<std::string>& GetCommands()
        { return commands_; }

        std::size_t GetAccountId() const
        { return account_; }

        std::size_t GetTaskId() const
        { return task_; }

        std::size_t GetConnId() const
        { return conn_; }

        std::size_t GetCmdListId() const
        { return id_; }

        Type GetType() const
        { return cmdtype_; }

        // cancel the cmdlist. the cmdlist is to be discarded.
        // i.e. the task has been cancelled.
        void Cancel()
        { cancelbit_ = true; }

        bool IsCancelled() const
        { return cancelbit_;}

        bool IsGood() const
        { return !failbit_; }

        bool IsEmpty() const
        { return buffers_.empty(); }

        void SetAccountId(std::size_t aid)
        { account_ = aid; }

        void SetTaskId(std::size_t tid)
        { task_ = tid; }

        void SetConnId(std::size_t cid)
        { conn_ = cid; }

        // returns whether the CmdList can be filled from another
        // account, i.e. whether the content's are available on any server
        bool IsFillable() const
        {
            // only article downloads using global message ids can be
            // filled, i.e. obtained from any server.
            if (cmdtype_ != Type::Article)
                return false;

            // check the article identifcation strings.
            // articles can be using either article number
            // or a message id. The message ids are unique
            // across all servers and are enclosed in angle brackets
            // for example <foboar@keke.boo.blah>
            for (const auto& cmd : commands_)
            {
                if (cmd.front() != '<' || cmd.back() != '>')
                    return false;
            }
            return true;
        }

    private:
        CmdList() : cancelbit_(false), failbit_(false), account_(0), task_(0), conn_(0)
        {
            static std::atomic<std::size_t> id(1);
            id_ = id++;
        }
    private:
        // cmdlist data
        Type cmdtype_ = Type::Header;
        std::atomic<bool> cancelbit_;
        std::atomic<bool> failbit_;
    private:
        // owner account, task and connection
        std::size_t account_ = 0;
        std::size_t task_ = 0;
        std::size_t conn_ = 0;
        std::size_t id_   = 0;
    private:
        // nntp data
        std::vector<buffer> buffers_;
        std::vector<std::string> groups_;
        std::vector<std::string> commands_;
    private:
    };

} // newsflash
