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

#include <functional>
#include <string>
#include <memory>
#include <deque>
#include <cstddef>

namespace newsflash
{
    class buffer;

    // session encapsulates the details of nntp. it provides a simple
    // api to request data and state changes in the session.
    // each such request might result in multiple pending commands in the 
    // session pipeline. thus the client should call parse_next repeatedly
    // while there are pending commands in the session and fill the input
    // buffer with more data between calls to parse_next
    class session
    {
    public:
        enum class error {
            none, 

            // possibly incorrect username/password pair            
            authentication_rejected, 

            // possibly out of quota 
            no_permission 
        };

        enum class state {
            // initial state            
            none, 

            // session is initializing with initial greeting etc.
            init, 

            // session is authenticating 
            authenticate, 

            // session is ready and there are no pending commands
            ready, 

            // session is performing data transfer 
            // such as getting article data, overview data or group (listing) data
            transfer, 

            // session is quitting
            quitting, 

            // an error has occurred. see error enum for details
            error
        };

        // called when session wants to send data
        std::function<void (const std::string& cmd)> on_send;

        // called when session wants to authenticate
        std::function<void (std::string& user, std::string& pass)> on_auth;

        session();
       ~session();

        // reset session state to none
        void reset();

        // start new session. this prepares the session pipeline
        // with initial session start commands.
        void start();

        // quit the session. prepares a quit command 
        void quit();

        // request to change the currently selected newsgroup 
        // to the new group identified by name. the result will be a
        // groupinfo buffer with group details (low/high water mark + total article count)
        // in the buffer header.
        void change_group(std::string name);

        // retrieve the article identified by the given messageid.
        // the result will be an article buffer with content carrying
        // the article body.
        void retrieve_article(std::string messageid);

        // send next queued command.
        // returns true if next command was sent otherwise false.
        bool send_next();

        // parse the buff for input data and try to complete
        // currently pending session command. 
        // if the command is succesfully completed returns true
        // in which case the session state should be inspected
        // next for possible errors. Also if the current command
        // is transferring content data the contents are placed
        // into the given out buffer. 
        // otherwise if the current command could not be completed
        // the function returns false.
        bool recv_next(buffer& buff, buffer& out);

        // clear pending commands.
        void clear();

        // returns true if there are pending commands. i.e. 
        // more calls to parse_next are required to complete
        // the session state changes.
        bool pending() const;

        void enable_pipelining(bool on_off);

        void enable_compression(bool on_off);

        // get current error
        error get_error() const;

        // get current state
        state get_state() const;

        // true if server supports zlib compression
        bool has_gzip_compress() const;

        // true if server supports xzver i.e. compressed headers.
        bool has_xzver() const;

    private:
        void submit_next_commands();

    private:
        struct impl;

        class command;
        class welcome;
        class getcaps;
        class modereader;
        class authuser;
        class authpass;
        class group;
        class body;
        class quit;
        class xover;
        class xzver;

    private:
        std::deque<std::unique_ptr<command>> send_;
        std::deque<std::unique_ptr<command>> recv_;        
        std::unique_ptr<impl> state_;
    };

} // newsflash