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
#include "nntp/nntp.h"

namespace newsflash
{
    class buffer;

    // implements NNTP protocol state
    class protocol
    {
    public:
        enum class error {
            authentication_failed,
            service_temporarily_unavailable,
            service_permanently_unavailable
        };

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
            exception(std::string what, error err) NOTHROW
                : what_(std::move(what)), err_(err)
            {}
            const char* what() const NOTHROW
            {
                return what_.c_str();
            }
            error code() const NOTHROW
            {
                return err_;
            }
        private:
            const std::string what_;
            const error err_;
        };

        // read protocol data in callback.
        std::function<size_t (void* buff, std::size_t len)> on_recv;

        // outgoing protocol data callback
        std::function<void (const void* buff, std::size_t len)> on_send;

        // write something in the log
        std::function<void (const std::string& str)> on_log;

        // authentication requested.
        std::function<void (std::string& user, std::string& pass)> on_auth;

        enum class status {
            success, unavailable, dmca
        };

        struct groupinfo {
            uint64_t high_water_mark;
            uint64_t low_water_mark;
            uint64_t article_count;
        };

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

        // query group information. returns true if such group exists
        // and information could be retrieved. otherwise false.
        bool query_group(const std::string& groupname, groupinfo& info);
        
        // download the contents of the article identified by article number or id.
        // returns true if article was downloaded succesfully, false if 
        // it was no longer available.
        status download_article(const std::string& article, buffer& buff);

        // download overview overview data in the first last range (inclusive)        
        // first and last are article numbers
        bool download_overview(const std::string& first, const std::string& last, buffer& buff);

        // download list of available newsgroups.
        bool download_list(buffer& buff);

    private:
        template<typename Cmd>
        auto transact(Cmd* cmd) -> decltype(cmd->transact())
        {
            cmd->recv = on_recv;
            cmd->send = on_send;
            cmd->log  = on_log;

            auto ret = cmd->transact();
            if (ret == nntp::AUTHENTICATION_REQUIRED)
            {
                authenticate();
                ret = cmd->transact();
            }
            return ret;
        }

        template<typename Cmd>
        auto transact(Cmd cmd) -> decltype(cmd.transact())
        {
            return transact(&cmd);
        }    

        void authenticate();


        bool has_compress_gzip_;
        bool has_xzver_;
        bool has_mode_reader_;
        bool is_compressed_;
        std::string group_; // selected group

    };

} // newsflash
