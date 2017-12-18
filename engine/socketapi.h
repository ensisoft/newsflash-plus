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

#include <system_error>
#include <string>
#include <cstdint>
#include <utility> // for pair

#include "native_types.h"

namespace newsflash
{
    using ipv4addr_t = std::uint32_t;
    using ipv4port_t = std::uint16_t;

    // resolve the host name and return the first address in network byte order
    // or 0 if host could not be resolved. (use socket_error() to query for error)
    std::error_code resolve_host_ipv4(const std::string& hostname,
        std::uint32_t& addr);

    native_handle_t get_wait_handle(native_socket_t sock);

    // initiate a new stream (TCP) based connection to the given host.
    // returns the new socket and wait object handles. The socket is
    // initially non-blocking. Check the wait handle for completion of the connection attempt.
    std::pair<native_socket_t, native_handle_t> begin_socket_connect(ipv4addr_t host, ipv4port_t port);

    // complete the previously started connection attempt.
    // if connection was succesful this also makes the socket blocking again.
    // on error an exception is thrown.
    std::error_code complete_socket_connect(native_handle_t handle, native_socket_t sock);

    // get the last occurred socket error.
    std::error_code get_last_socket_error();

    void closesocket(native_handle_t handle, native_socket_t sock);

    void closesocket(native_socket_t sock);

} // newsflash
