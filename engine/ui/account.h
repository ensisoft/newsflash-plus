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

#include <string>
#include <cstddef>
#include <cstdint>
#include "server.h"

namespace newsflash
{
    namespace ui {

    // newsserver account. 
    struct account 
    {
        // unique id for the account. used to refer to the
        // account in the newsflash api.
        std::size_t id;

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

        std::string secure_host;

        std::string general_host;

        std::uint16_t secure_port;

        std::uint16_t general_port;

        // maximum number of connections to be opened
        // to the servers under this account.
        int connections;

        bool enable_secure_server;

        bool enable_general_server;

        // todo:
        bool enable_compression;

        // todo:
        bool enable_pipelining;

        // set to true to specify this account as a "fill"
        // (secondary) account. a fill account is consulted
        // when the primary server is missing some data.
        //bool is_fill_account;
    };

} // ui
}// engine
