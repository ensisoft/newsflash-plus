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

#include <newsflash/config.h>

#include <string>
#include <cstddef>

namespace newsflash
{
    namespace ui {

    // newsserver configuration
    struct server 
    {
        // hostname or IP4/6 address.
        std::string host; 

        // the port number. typically Usenet servers
        // run on port 119 for the general non-encrypted
        // server and 443 or 563 for an encrypted server
        // if that is supported.
        std::uint16_t port; // port

        // true if ipv6 should be used.
        bool ipv6;          

    };

    inline
    bool is_valid(const server& server)
    {
        return !server.host.empty() && server.port;
    }

} // ui
} // engine
