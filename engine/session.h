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

#include <functional>
#include <string>
#include <memory>
#include <deque>
#include <cstddef>

namespace newsflash
{
    class Buffer;

    // session encapsulates the details of nntp. it provides a simple
    // api to request data and state changes in the session.
    // each such request might result in multiple pending commands in the
    // session pipeline. thus the client should call parse_next repeatedly
    // while there are pending commands in the session and fill the input
    // buffer with more data between calls to parse_next
    class Session
    {
    public:
        enum class Error {
            None,

            // incorrect protocol, maybe the other end is not NNTP?
            Protocol,

            // possibly incorrect username/password pair
            AuthenticationRejected,

            // possibly out of quota
            NoPermission
        };

        enum class State {
            // initial state
            None,

            // session is initializing with initial greeting etc.
            Init,

            // session is authenticating
            Authenticate,

            // session is ready and there are no pending commands
            Ready,

            // session is performing data transfer
            // such as getting article data, overview data or group (listing) data
            Transfer,

            // session is quitting
            Quitting,

            // an error has occurred. see error enum for details
            Error
        };

        // callback when session wants to send data
        using SendCallback = std::function<void(const std::string& cmd)>;


        Session();
       ~Session();

        // reset session state to none
        void Reset();

        // start new session. this prepares the session pipeline
        // with initial session start commands.
        void Start(bool authenticate_immediately = false);

        // quit the session. prepares a quit command
        void Quit();

        // request to change the currently selected newsgroup to the new group
        void ChangeGroup(const std::string& name);

        // retrive group information. the result will be a groupinfo buffer
        // with high and low water marks for article numbers.
        void RetrieveGroupInfo(const std::string& name);

        // retrieve the article identified by the given messageid.
        // the result will be an article buffer with content carrying
        // the article body.
        void RetrieveArticle(const std::string& messageid);

        // retrieve the headers in the specified range [start - end]
        void RetrieveHeaders(const std::string& range);

        // retrieve newsgroup listing
        void RetrieveList();

        // do a ping to keep the session alive.
        void Ping();

        // send next queued command.
        // returns true if next command was sent otherwise false.
        bool SendNext();

        // parse the buff for input data and try to complete
        // currently pending session command.
        // if the command is succesfully completed returns true
        // in which case the session state should be inspected
        // next for possible errors. Also if the current command
        // is transferring content data the contents are placed
        // into the given out buffer.
        // otherwise if the current command could not be completed
        // the function returns false.
        bool RecvNext(Buffer& buff, Buffer& out);

        // clear pending commands.
        void Clear();

        // returns true if there are pending commands. i.e.
        // more calls to parse_next are required to complete
        // the session state changes.
        bool HasPending() const;

        // turn on/off command pipelining where applicable.
        void SetEnablePipelining(bool on_off);

        // turn on/off header compression.
        void SetEnableCompression(bool on_off);

        // set the session user credentials
        void SetCredentials(const std::string& username, const std::string& password);

        // set the send callback to be invoked when session
        // needs to send some data.
        // This must be set.
        void SetSendCallback(const SendCallback& callback);

        // get current error
        Error GetError() const;

        // get current state
        State GetState() const;

        // true if server supports zlib compression
        bool HasGzipCompress() const;

        // true if server supports xzver i.e. compressed headers.
        bool HasXzver() const;

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
        class xovergzip;
        class list;
        class xfeature_compress_gzip;

    private:
        std::deque<std::unique_ptr<command>> send_;
        std::deque<std::unique_ptr<command>> recv_;
        std::unique_ptr<impl> state_;
    };

} // newsflash
