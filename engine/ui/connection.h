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
#include <string>

namespace newsflash
{
    namespace ui {

    // a connection managed by the engine.
    struct connection
    {
        // possible connection errors.
        enum class errors {
            // no error
            none,

            // could not resolve host. this could because the theres a DNS problem
            // in the network, or the network itself is down or the hostname 
            // is not known by the DNS resolver.
            resolve,

            // connection was refused by the remote party.
            refused,

            // connection was made to the server but the server
            // denied access based on incorrect authentication.
            // possibly username/password is wrong
            authentication_rejected,

            // connection was made to the server but the server
            // denied access. possibly the account is locked/out of quota.
            no_permission,

            // network problem, for example network is down.
            network,

            // active timeout, usully preceeds a network problem.
            timeout,

            other
        }; 

        enum class states {
            // disconnected state
            disconnected,

            // connection is resolving the host address 
            resolving,

            // connection is connecting to the remote host.
            connecting,

            // connection is initializing the protocol stack
            initializing,

            // connection is connected, idle and ready.
            connected,

            // connection is transferring data.
            active,

            error
        };

        // current error if any.
        errors error;

        // current state.
        states state;

        // unique connection id
        std::size_t id;

        // the task id currently being processed.
        std::size_t task;

        // the account to which this connection is connected to.
        std::size_t account;

        // total bytes downloaded.
        std::uint64_t down;

        // current host
        std::string host;

        std::string logfile;

        std::uint16_t port;

        // current description.
        std::string desc;

        // secure or not
        bool secure;

        // current speed in bytes per second.
        std::uint32_t bps;
    };

} // ui
} // engine
