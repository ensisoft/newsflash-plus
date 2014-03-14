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

#include "cmd.h"
#include "transact.h"

namespace nntp
{
    // send AUTHINFO PASS to authenticate the current session.
    // should only be sent when 318 password required is received.
    // 281 authentication accepted
    // 381 password required
    // 481 authentication failed/rejected
    // 482 authentication command issued out of sequence
    // 502 command unavailable 
    struct cmd_auth_pass : cmd {

        const std::string pass;

        cmd_auth_pass(std::string password) : pass(std::move(password))
        {}

        code_t transact()
        {
            return nntp::transact(*this, "AUTHINFO PASS " + pass, {281, 381, 481, 482, 502});
        }
    };

} // nntp
