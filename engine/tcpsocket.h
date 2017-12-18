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

#include "utility.h"
#include "socket.h"

namespace newsflash
{
    // transfer data over raw TCP socket.
    class TcpSocket : public Socket
    {
    public:
        // construct a non-connected socket.
        TcpSocket()
        {}

        TcpSocket(const TcpSocket&) = delete;

        // take ownership of the given socket and handle.
        // the socket is expected to be already connected.
        TcpSocket(native_socket_t sock, native_handle_t handle)
          : socket_(sock)
          , handle_(handle)
        {}
        TcpSocket(TcpSocket&& other)
          : socket_(other.socket_)
          , handle_(other.handle_)
        {
            other.socket_ = 0;
            other.handle_ = 0;
        }
       ~TcpSocket()
        {
            Close();
        }

        virtual void BeginConnect(ipv4addr_t host, ipv4port_t port) override;
        virtual std::error_code CompleteConnect() override;
        virtual void SendAll(const void* buff, int len) override;
        virtual int SendSome(const void* buff, int len) override;
        virtual int RecvSome(void* buff, int capacity) override;
        virtual void Close() override;
        virtual waithandle GetWaitHandle() const override;
        virtual waithandle GetWaitHandle(bool waitread, bool waitwrite) const override;
        virtual bool CanRecv() const override;

        TcpSocket& operator=(TcpSocket&& other);

        TcpSocket& operator=(const TcpSocket&) = delete;
    private:
        // the actual socket
        native_socket_t socket_ = 0;

        // event handle associated with the socket
        native_handle_t handle_ = 0;
    };

} // newsflash
