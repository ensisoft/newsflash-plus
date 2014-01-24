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

#include <exception>
#include <functional>
#include <string>
#include "protocmd.h"

namespace newsflash
{
    class buffer;

    // implements NNTP protocol state
    class protocol
    {
    public:
        // this gets thrown when things go wrong.
        // you might wonder why exceptions for conditions that
        // are not really "errors"? but this makes it easy to
        // write code that only needs to concern itself with 
        // soft logical problems such as "group not available"
        // or "article not availble." and such conditions can easily
        // be identified by a return value.
        class exception : public std::exception
        {
        public:
            enum class code {
                authentication_failed,
                service_temporarily_unavailable,
                service_permanently_unavailable
            };
            exception(std::string what, exception::code code) NOTHROW
                : what_(std::move(what)), code_(code)
            {}
            const char* what() const NOTHROW
            {
                return what_.c_str();
            }
            code error() const NOTHROW
            {
                return code_;
            }
        private:
            const std::string what_;
            const exception::code code_;
        };

        // read protocol data in callback.
        std::function<size_t (void*, size_t)>    on_recv;

        // outgoing protocol data callback
        std::function<void (const void*, size_t)> on_send;

        // write something in the log
        std::function<void (const std::string&)> on_log;

        // authentication requested.
        std::function<void (std::string& user, 
                            std::string& pass)> on_authenticate;

        protocol();
       ~protocol();

        // (re)start protocol state from connecting state.
        void connect();       

        // send ping to keep protocol state alive
        void ping(); 

        // send quit
        void quit();

        // change the currently active group to the newgroup.
        // returns true if change was succesful, otherwise false.
        bool change_group(const std::string& groupname);

        struct groupinfo {
            uint64_t high_water_mark;
            uint64_t low_water_mark;
            uint64_t article_count;
        };

        std::pair<bool, groupinfo> query_group(const std::string& groupname);

        // download the contents of the article identified by article number or id.
        // returns true if article was downloaded succesfully, false if 
        // it was no longer available.
        bool download_article(const std::string& article, buffer& buff);

        // download overview overview data in the first last range (inclusive)        
        // first and last are article numbers
        bool download_overview(const std::string& first, const std::string& last, buffer& buff);

        // download list of available newsgroups
        bool download_list(buffer& buff);

    private:
        template<typename Cmd>
        auto transact(Cmd* cmd) -> decltype(cmd->transact())
        {
            cmd->cmd_recv = on_recv;
            cmd->cmd_send = on_send;
            cmd->cmd_log  = on_log;

            auto ret = cmd->transact();
            if (ret == nntp::AUTHENTICATION_REQUIRED)
            {
                ret = authenticate();
                if (ret != nntp::AUTHENTICATION_ACCEPTED)
                    throw exception("authentication failed", exception::code::authentication_failed);

                // ask for caps again cause they might be reported
                // differently now that we're authenticated.
                querycaps();

                ret = cmd->transact();
            }
            if (ret == nntp::SERVICE_UNAVAILABLE)
                throw exception("service unavailable", exception::code::service_temporarily_unavailable);

            return ret;
        }

        template<typename Cmd>
        auto transact(Cmd cmd) -> decltype(cmd.transact())
        {
            return transact(&cmd);
        }    

        int authenticate();
        int querycaps();


        bool has_compress_gzip_;
        bool has_xzver_;
        bool is_compressed_;
        std::string group_; // selected group

    };

} // newsflash
