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

#include <cstdint>
#include <cstddef>
#include <string>

namespace newsflash
{
    namespace ui {

    // a connection managed by the engine.
    struct Connection
    {
        // possible connection errors.
        enum class Errors {
            // no error
            None,

            // could not resolve host. this could because the theres a DNS problem
            // in the network, or the network itself is down or the hostname
            // is not known by the DNS resolver.
            Resolve,

            // connection was refused by the remote party.
            Refused,

            // network problem, for example network is down.
            Network,

            // connection was made to the server but the server
            // denied access based on incorrect authentication.
            // possibly username/password is wrong
            AuthenticationRejected,

            // connection was made to the server but the server
            // denied access. possibly the account is locked/out of quota.
            NoPermission,

            // active timeout, usully preceeds a network problem.
            Timeout,

            // some other error. details are not known.
            Other
        };

        enum class States {
            // disconnected state
            Disconnected,

            // connection is resolving the host address
            Resolving,

            // connection is connecting to the remote host.
            Connecting,

            // connection is initializing the protocol stack
            Initializing,

            // connection is connected, idle and ready.
            Connected,

            // connection is transferring data.
            Active,

            // connection has encountered an error.
            Error
        };

        // current error if any.
        Errors error = Errors::None;

        // current state.
        States state = States::Disconnected;

        // unique connection id
        std::size_t id = 0;

        // the task id currently being processed. 0 if no task.
        std::size_t task = 0;

        // the account to which this connection is connected to.
        std::size_t account = 0;

        // total bytes downloaded.
        std::uint64_t down = 0;

        // current host
        std::string host;

        // path to the log file
        std::string logfile;

        std::uint16_t port = 563;

        // current description.
        std::string desc;

        // secure or not
        bool secure = true;

        // current speed in bytes per second.
        std::uint32_t bps = 0;

    };

} // ui
} // engine
