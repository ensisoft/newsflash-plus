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

#include <stdexcept>
#include <string>
#include <memory>
#include <functional>

#include <cstdint>
#include "action.h"

namespace newsflash
{
    class cmdlist;
    class throttle;

    class connection
    {
    public:
        enum class error {
            none,
            resolve,
            refused,
            reset,
            protocol,
            authentication_rejected,
            no_permission,
            timeout,
            network,
            pipeline_reset
        };

        enum class state {
            disconnected,
            resolving,
            connecting,
            initializing,
            connected,
            active,
            error
        };

        struct cmdlist_completion_data {
            std::shared_ptr<cmdlist> cmds;
            std::size_t task_owner_id = 0;
            std::uint64_t total_bytes = 0;
            std::uint64_t content_bytes = 0;
            bool success = false;
        };

        using on_cmdlist_done = std::function<void (const cmdlist_completion_data&)>;

        struct spec {
            std::string username;
            std::string password;
            std::string hostname;
            std::uint16_t hostport = 563;
            bool use_ssl = true;
            bool enable_pipelining = false;
            bool enable_compression = false;
            throttle* pthrottle = nullptr;
            bool authenticate_immediately = false;
        };

        connection();

        // begin connecting to the given host specification.
        std::unique_ptr<action> connect(const spec& s);

        // perform disconnect from the host.
        std::unique_ptr<action> disconnect();

        // perform ping
        std::unique_ptr<action> ping();

        // complete the given action. returns a new action if any.
        std::unique_ptr<action> complete(std::unique_ptr<action> a);

        // execute the given cmdlist
        std::unique_ptr<action> execute(std::shared_ptr<cmdlist> cmd, std::size_t tid);

        // cancel a pending operation in the connection such as connecting
        // or executing a cmdlist. note that if the operation is excute
        // cancelling the connection does not cancel the cmdlist!
        void cancel();

        // get the current username specified for the connection.
        std::string username() const;

        // get the current password specified for the connection.
        std::string password() const;

        // get the current transfer speed in bytes per second.
        std::uint32_t current_speed_bps() const;

        // get the total number of payload bytes transferred.
        // i.e. this is only the actual content/data excluding any
        // protocol/SSL bytes.
        std::uint64_t num_bytes_transferred() const;

        // get the current connection state.
        state get_state() const;

        // get the current error when state indicates an error
        error get_error() const;

        // set the callback to be invoked when an action
        // to download a cmdlist has been completed.
        void set_callback(const on_cmdlist_done& callback);

    private:
        struct impl;
        class resolve;
        class connect;
        class initialize;
        class execute;
        class disconnect;
        class ping;

    private:
        std::shared_ptr<impl> state_;

    };

} // newsflash


