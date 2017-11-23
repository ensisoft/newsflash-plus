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

#include <string>
#include <cstddef>
#include <cstdint>

namespace newsflash
{
    namespace ui {

    // news account
    struct Account
    {
        // unique id for the account. used to refer to the
        // account in the newsflash api.
        std::size_t id = 0;

        // name for the account
        std::string name;

        // username if user credentials are required.
        std::string username;

        // password if user credentials are required.
        std::string password;

        // typically newsservice providers provide multiple
        // servers with different hostnames and ports.
        // we allow the account to be configured
        // with 2 servers. one for the general unecrypted use
        // and the secure server that uses encryption.

        // encrypted host for SSL over TCP connection.
        std::string secure_host;

        // unencrypted host for TCP connection.
        std::string general_host;

        // encrypted host port for SSL over TCP connection.
        std::uint16_t secure_port = 563;

        // unencrypted host port for TCP connection.
        std::uint16_t general_port = 119;

        // maximum number of connections to be opened
        // to the servers under this account.
        std::uint16_t connections = 1;

        // true if the secure server setting is enabled.
        bool enable_secure_server = true;

        // true if the non-secure server setting is enabled.
        bool enable_general_server = false;

        // try to enable and use compressed headers.
        // note that not all servers support this.
        bool enable_compression = false;

        // enable command pipelining. some NNTP commands
        // can be sent in a pipeline and then multiple reponses read
        // instead of executing each command as a request/response pair.
        // on some hosts command pipelining *may* improve performance.
        bool enable_pipelining = false;

        // user specific opaque data object that will be associated
        // with *all* the connection objects created for this account.
        // the client is responsinble for managin the lifetime of this
        // data object.
        void* user_data = nullptr;
    };

    } // ui
}// engine
