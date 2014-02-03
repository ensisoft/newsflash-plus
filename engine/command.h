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

#include <memory>
#include <string>
#include <cstdint>

namespace newsflash
{
    class buffer;

    struct command {
        enum class cmdtype {
            body, group, xover, list
        };
        cmdtype type;
        size_t  id;
        size_t  taskid;
        size_t  size; // expected buffer size        

        virtual ~command() = default;

        command() : id(0), taskid(0), size(0)
        {}
    };

    // Request to retrieve the message body of the 
    // article specified by message id or by article number.
    // If message is no longer available on the server
    // the status will be set to "unavailable". If the reason
    // for unvailability was identified to be a "dmca" takedown
    // the status is set to dmca. On successful retrieval
    // of the body the status is set to success.
    struct cmd_body : public command {
        enum class cmdstatus {
            unavailable, dmca, success,
        };
        cmdstatus status;
        std::string article; // either <message-id> or article number.        
        std::string groups[3]; // groups to look into for this message
        std::shared_ptr<buffer> data;

        cmd_body(std::string id, std::string group) : status(cmdstatus::unavailable),
            article(std::move(id))
        {
            type = cmdtype::body;
            size = 1024 * 1024; // 1mb
            groups[0] = std::move(group);
        }
    };

    // Request information about a particular newsgroup.
    // on successful completion of the command high and low
    // water marks identicate the lowest and highest available
    // article numbers in the group. Article count gives
    // an estimate of available articles. 
    struct cmd_group : public command {
        std::string name;
        std::uint64_t high_water_mark;
        std::uint64_t low_water_mark;
        std::uint64_t article_count;
        bool success;

        cmd_group(std::string groupname) 
            : name(std::move(groupname)),
              high_water_mark(0), low_water_mark(0), article_count(0), success(false)
        {
            type = cmdtype::group;
        }
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
    struct cmd_xover : public command {
        std::string group;
        std::size_t start;
        std::size_t end;
        std::shared_ptr<buffer> data;
        bool success;

        cmd_xover(std::string groupname, std::size_t startrange, std::size_t endrange)
            : group(std::move(groupname)), start(startrange), end(endrange), success(false)
        {
            type = cmdtype::xover;
            size = 1024 * 512;
        }
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
    struct cmd_list : public command {
        std::shared_ptr<buffer> data;

        cmd_list()
        {
            type = cmdtype::list;
            size = 1024 * 5;
        }
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
