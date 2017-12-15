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
    class CmdList;
    class throttle;

    class Connection
    {
    public:
        struct HostDetails {
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

        enum class Error {
            None,
            Resolve,
            Refused,
            Reset,
            Protocol,
            AuthenticationRejected,
            PermissionDenied,
            Timeout,
            Network,
            PipelineReset
        };

        enum class State {
            Disconnected,
            Resolving,
            Connecting,
            Initializing,
            Connected,
            Active,
            Error
        };


        virtual ~Connection() = default;

        // begin connecting to the given host specification.
        virtual std::unique_ptr<action> Connect(const HostDetails& host) = 0;

        // perform disconnect from the host.
        virtual std::unique_ptr<action> Disconnect() = 0;

        // perform ping
        virtual std::unique_ptr<action> Ping() = 0;

        // complete the given action. returns a new action if any.
        virtual std::unique_ptr<action> Complete(std::unique_ptr<action> a) = 0;

        // execute the given cmdlist
        virtual std::unique_ptr<action> Execute(std::shared_ptr<CmdList> cmd, std::size_t tid) = 0;

        // cancel a pending operation in the connection such as connecting
        // or executing a cmdlist. note that if the operation is excute
        // cancelling the connection does not cancel the cmdlist!
        virtual void Cancel() = 0;

        // get the current username specified for the connection.
        virtual std::string GetUsername() const = 0;

        // get the current password specified for the connection.
        virtual std::string GetPassword() const = 0;

        // get the current transfer speed in bytes per second.
        virtual std::uint32_t GetCurrentSpeedBps() const = 0;

        // get the total number of payload bytes transferred.
        // i.e. this is only the actual content/data excluding any
        // protocol/SSL bytes.
        virtual std::uint64_t GetNumBytesTransferred() const = 0;

        // get the current connection state.
        virtual State GetState() const = 0;

        // get the current error when state indicates an error
        virtual Error GetError() const = 0;

        struct CmdListCompletionData {
            std::shared_ptr<CmdList> cmds;
            std::size_t task_owner_id = 0;
            std::uint64_t total_bytes = 0;
            std::uint64_t content_bytes = 0;
            bool execution_did_complete = false;
        };

        using OnCmdlistDone = std::function<void (const CmdListCompletionData&)>;


        // set the callback to be invoked when an action
        // to download a cmdlist has been completed.
        virtual void SetCallback(const OnCmdlistDone& callback) = 0;

    };

    class ConnectionImpl : public Connection
    {
    public:
        ConnectionImpl();

        // begin connecting to the given host specification.
        virtual std::unique_ptr<action> Connect(const HostDetails& host) override;

        // perform disconnect from the host.
        virtual std::unique_ptr<action> Disconnect() override;

        // perform ping
        virtual std::unique_ptr<action> Ping() override;

        // complete the given action. returns a new action if any.
        virtual std::unique_ptr<action> Complete(std::unique_ptr<action> a) override;

        // execute the given cmdlist
        virtual std::unique_ptr<action> Execute(std::shared_ptr<CmdList> cmd, std::size_t tid) override;

        // cancel a pending operation in the connection such as connecting
        // or executing a cmdlist. note that if the operation is excute
        // cancelling the connection does not cancel the cmdlist!
        virtual void Cancel() override;

        // get the current username specified for the connection.
        virtual std::string GetUsername() const override;

        // get the current password specified for the connection.
        virtual std::string GetPassword() const override;

        // get the current transfer speed in bytes per second.
        virtual std::uint32_t GetCurrentSpeedBps() const override;

        // get the total number of payload bytes transferred.
        // i.e. this is only the actual content/data excluding any
        // protocol/SSL bytes.
        virtual std::uint64_t GetNumBytesTransferred() const override;

        // get the current connection state.
        virtual State GetState() const override;

        // get the current error when state indicates an error
        virtual Error GetError() const override;

        // set the callback to be invoked when an action
        // to download a cmdlist has been completed.
        void SetCallback(const OnCmdlistDone& callback) override;

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


