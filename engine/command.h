// Copyright (c) 2013,2014 Sami Väisänen, Ensisoft 
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

#include <boost/lexical_cast.hpp>
#include <memory>
#include <string>
#include <cstdint>
#include "buffer.h"
#include "protocol.h"

namespace newsflash
{
    class buffer;

    // abstract NNTP command to be performed
    class command 
    {
    public:
        enum class cmdtype {
            body, group, xover, list
        };

        command(std::size_t id, 
                std::size_t task) : id_(id), taskid_(task)
        {}

        virtual ~command() = default;

        // perform the command against the protocol object
        virtual void perform(protocol& proto) = 0;

        // get the command type
        virtual cmdtype type() const = 0;

        std::size_t id() const
        {
            return id_;
        }
        std::size_t task() const
        {
            return taskid_;
        }

    private:
        const std::size_t id_;
        const std::size_t taskid_;
    };

    // Request to retrieve the message body of the 
    // article specified by message id or by article number.
    // If message is no longer available on the server
    // the status will be set to "unavailable". If the reason
    // for unvailability was identified to be a "dmca" takedown
    // the status is set to dmca. On successful retrieval
    // of the body the status is set to success.
    class cmd_body : public command
    {
    public:
        enum class cmdstatus {
            unavailable, dmca, success
        };

        cmd_body(std::size_t id, 
                 std::size_t task,
                 std::string article,
                 std::vector<std::string> groups,
                 std::size_t size = 1024 * 1024) : command(id, task),
          article_(std::move(article)),
          groups_(std::move(groups)),
          size_(size),
          status_(cmdstatus::unavailable)
        {}

        virtual void perform(protocol& proto) override
        {
            buff_ = std::make_shared<buffer>();
            buff_->allocate(size_);
            for (const auto& group : groups_)
            {
                if (group.empty())
                    continue;

                const auto ret = proto.download_article(article_, *buff_);
                if (ret == protocol::status::success)
                    status_ = cmdstatus::success;
                else if (ret == protocol::status::dmca)
                    status_ = cmdstatus::dmca;
                else if (ret == protocol::status::unavailable)
                    status_ = cmdstatus::unavailable;

                if (status_ != cmdstatus::unavailable)
                    break;
            }
        }

        virtual cmdtype type() const override
        {
            return cmdtype::body;
        }
        cmdstatus status() const
        {
            return status_;
        }
        bool empty() const
        {
            return buff_->empty();
        }
        const std::string& article() const
        {
            return article_;
        }
        
    private:
        const std::string article_; // <message-id> or article number
        const std::vector<std::string> groups_; // groups to look into for this message
        const std::size_t size_; // expected body size
        std::shared_ptr<buffer> buff_;
        cmdstatus status_;
    };


    // Request information about a particular newsgroup.
    // on successful completion of the command high and low
    // water marks identicate the lowest and highest available
    // article numbers in the group. Article count gives
    // an estimate of available articles. 
    class cmd_group : public command
    {
    public:
        cmd_group(std::size_t id,
                  std::size_t task,
                  std::string group) : command(id, task),
            name_(std::move(group)),
            high_water_mark_(0),
            low_water_mark_(0),
            article_count_(0),
            success_(false)
            {}

        virtual void perform(protocol& proto) override
        {
            protocol::groupinfo info {0};
            success_ = proto.query_group(name_, info);
            if (success_)
            {
                high_water_mark_ = info.high_water_mark;
                low_water_mark_  = info.low_water_mark;
                article_count_   = info.article_count;
            }
        }
        virtual cmdtype type() const override
        {
            return cmdtype::group;
        }

        bool success() const 
        {
            return success_;
        }

        std::uint64_t high() const
        {
            return high_water_mark_;
        }

        std::uint64_t low() const
        {
            return low_water_mark_;
        }

        std::uint64_t count() const
        {
            return article_count_;
        }
    private:
        const std::string name_;
        std::uint64_t high_water_mark_;
        std::uint64_t low_water_mark_;
        std::uint64_t article_count_;
        bool success_;
    };

    // Request overview information for the articles in the
    // [start, end] range (inclusive). On succesful completion
    // of the command the returned data contains article overview
    // data in the overview (\t delimeted) format.
    //
    // Each line of output will be formatted with the article number,
    // followed by each of the headers in the overview database or the
    // article itself (when the data is not available in the overview
    // database) for that article separated by a tab character.  The
    // sequence of fields must be in this order: subject, author, date,
    // message-id, references, byte count, and line count.  Other optional
    // fields may follow line count.  Other optional fields may follow line
    // count.  These fields are specified by examining the response to the
    // LIST OVERVIEW.FMT command.  Where no data exists, a null field must
    // be provided (i.e. the output will have two tab characters adjacent to
    // each other).  Servers should not output fields for articles that have
    // been removed since the XOVER database was created.    
    class cmd_xover : public command
    {
    public:
        cmd_xover(std::size_t id, 
                  std::size_t task,
                  std::string group,
                  std::size_t start, 
                  std::size_t end) : command(id, task), 
            group_(std::move(group)),
            start_(start),
            end_(end),
            success_(false)
        {}

        virtual void perform(protocol& proto) override
        {
            success_ = proto.change_group(group_);
            if (success_)
            {
                buff_ = std::make_shared<buffer>();
                buff_->allocate(1024 * 5);

                const std::string& start = boost::lexical_cast<std::string>(start_);
                const std::string& end   = boost::lexical_cast<std::string>(end_);

                proto.download_overview(start, end, *buff_);
            }
        }
        virtual cmdtype type() const override
        {
            return cmdtype::xover;
        }
        bool empty() const
        {
            return buff_->empty();
        }

        bool success() const
        {
            return success_;
        }

    private:
        const std::string group_;
        const std::size_t start_;
        const std::size_t end_;
        std::shared_ptr<buffer> buff_;
        bool success_;
    };

    // Request a list of available newsgroups. 
    // The returned data is line separated and every line
    // contains the information for a single newsgroup.
    //
    // Each line has the following format:
    //    "group last first p"
    // where <group> is the name of the newsgroup, <last> is the number of
    // the last known article currently in that newsgroup, <first> is the
    // number of the first article currently in the newsgroup, and <p> is
    // either 'y' or 'n' indicating whether posting to this newsgroup is
    // allowed ('y') or prohibited ('n').
    //
    // The <first> and <last> fields will always be numeric.  They may have
    // leading zeros.  If the <last> field evaluates to less than the
    // <first> field, there are no articles currently on file in the
    // newsgroup.
    class cmd_list : public command
    {
    public:
        cmd_list(std::size_t id, 
                 std::size_t task) : command(id, task)
        {}

        virtual void perform(protocol& proto) override
        {
            buff_ = std::make_shared<buffer>();
            buff_->allocate(1024 * 1024);
            proto.download_list(*buff_);
        }

        virtual cmdtype type() const override
        {
            return cmdtype::list;
        }

        bool empty() const
        {
            return buff_->empty();
        }
    private:
        std::shared_ptr<buffer> buff_;
    };


    template<typename T>
    T& command_cast(std::unique_ptr<command>& cmd) NOTHROW
    {
#ifndef _NDEBUG
        return *dynamic_cast<T*>(cmd.get());
#else
        return *static_cast<T*>(cmd.get());
#endif
    }

} // newsflash
