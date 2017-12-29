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
#include <functional>

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
            buffers_.resize(1);
            commands_.push_back("dummy");
        }
        CmdList(const Messages& m) : CmdList()
        {
            groups_   = m.groups;
            commands_ = m.numbers;
            cmdtype_  = Type::Article;
            buffers_.resize(commands_.size());
        }
        CmdList(const Overviews& o) : CmdList()
        {
            groups_.push_back(o.group);
            commands_  = o.ranges;
            cmdtype_   = Type::Header;
            buffers_.resize(commands_.size());
        }
        CmdList(const GroupInfo& g) : CmdList()
        {
            groups_.push_back(g.group);
            cmdtype_ = Type::GroupInfo;
            buffers_.resize(1);
            commands_.push_back(g.group);
        }

        bool NeedsToConfigure() const
        {
            if (cmdtype_ == Type::Header || cmdtype_ == Type::Article)
                return true;

            return false;
        }

        // try the ith step to configure the session state
        bool SubmitConfigureCommand(std::size_t i, Session& ses)
        {
            if (i == groups_.size())
                return false;

            // try the ith group in the list of newsgroups that are supposed
            // to carry the articles. we stop after being able to succesfully
            // select whatever group comes first.
            ses.ChangeGroup(groups_[i]);
            return true;
        }

        // receive and handle ith response to ith configure command.
        bool ReceiveConfigureBuffer(std::size_t i, const Buffer& buff)
        {
            // if the group exists, it's good enough for us.
            if (buff.GetContentStatus() == Buffer::Status::Success)
                return true;

            // if we have no more groups to check, we're done.
            if (i == groups_.size() -1)
                failbit_ = true;

            return false;
        }

        // submit the data transfer commands as a single batch.
        void SubmitDataCommands(Session& ses)
        {
            ASSERT(commands_.size() == buffers_.size());
            for (size_t i=0; i<commands_.size(); ++i)
            {
                if (buffers_[i].GetContentStatus() == Buffer::Status::Success)
                    continue;

                if (cmdtype_ == Type::Listing)
                    ses.RetrieveList();
                else if (cmdtype_ == Type::GroupInfo)
                    ses.RetrieveGroupInfo(commands_[i]);
                else if (cmdtype_ == Type::Article)
                    ses.RetrieveArticle(commands_[i]);
                else if (cmdtype_ == Type::Header)
                    ses.RetrieveHeaders(commands_[i]);

                buffers_[i].Clear();
            }
        }

        // a single command version of SubmitDataCommands.
        // this is mostly to simplify testing.
        void SubmitDataCommand(std::size_t i, Session& session)
        {
            ASSERT(i < commands_.size());
            ASSERT(i < buffers_.size());

            if (buffers_[i].GetContentStatus() == Buffer::Status::Success)
                return;

            if (cmdtype_ == Type::Listing)
                session.RetrieveList();
            else if (cmdtype_ == Type::GroupInfo)
                session.RetrieveGroupInfo(commands_[i]);
            else if (cmdtype_ == Type::Article)
                session.RetrieveArticle(commands_[i]);
            else if (cmdtype_ == Type::Header)
                session.RetrieveHeaders(commands_[i]);

            buffers_[i].Clear();
        }


        // receive a data buffer response to submit_data_commands
        void ReceiveDataBuffer(const Buffer& buff)
        {
            for (auto& old : buffers_)
            {
                if (old.GetContentStatus() == Buffer::Status::None)
                {
                    old = buff;
                    return;
                }
            }
            ASSERT("No such buffer");
        }

        void ReceiveDataBuffer(Buffer&& buff)
        {
            for (auto& old : buffers_)
            {
                if (old.GetContentStatus() == Buffer::Status::None)
                {
                    old = std::move(buff);
                    return;
                }
            }
            ASSERT("No such buffer");
        }

        using ContentBufferCallback = std::function<void(const Buffer& buff, bool compressed)>;

        void InspectIntermediateContentBuffer(const Buffer& buffer, bool compressed)
        {
            if (!intermediate_content_buffer_callback_)
                return;
            intermediate_content_buffer_callback_(buffer, compressed);
        }

        void SetIntermediateContentBufferCallback(const ContentBufferCallback& callback)
        { intermediate_content_buffer_callback_ = callback; }

        // useful for testing purposes
        std::size_t NumGroups() const
        { return groups_.size(); }
        std::string GetGroup(std::size_t i) const
        { return groups_[i]; }

        std::size_t NumDataCommands() const
        { return commands_.size(); }

        std::size_t NumBuffers() const
        { return buffers_.size(); }

        std::vector<Buffer>& GetBuffers()
        { return buffers_; }

        Buffer& GetBuffer(std::size_t i)
        {
            ASSERT(i < buffers_.size());
            return buffers_[i];
        }

        const Buffer& GetBuffer(std::size_t i) const
        {
            ASSERT(i < buffers_.size());
            return buffers_[i];
        }

        const std::string& GetCommand(std::size_t i) const
        {
            ASSERT(i < commands_.size());
            return commands_[i];
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

        std::string GetDesc() const
        { return desc_; }

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

        bool IsActive() const
        { return conn_ != 0; }

        void SetAccountId(std::size_t aid)
        { account_ = aid; }

        void SetTaskId(std::size_t tid)
        { task_ = tid; }

        void SetConnId(std::size_t cid)
        { conn_ = cid; }

        void SetDesc(const std::string& description)
        { desc_ = description; }

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


        // returns true if some content could not be retrieved.
        bool HasFailedContent() const
        {
            for (const auto& buff : buffers_)
            {
                const auto status = buff.GetContentStatus();
                if (status == Buffer::Status::Unavailable ||
                    status == Buffer::Status::Dmca)
                    return true;
            }
            return false;
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
        std::string desc_;
    private:
        // nntp data
        std::vector<Buffer> buffers_;
        std::vector<std::string> groups_;
        std::vector<std::string> commands_;
    private:
        ContentBufferCallback intermediate_content_buffer_callback_;
    };

} // newsflash
