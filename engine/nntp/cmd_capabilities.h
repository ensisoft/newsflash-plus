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
#include "nntp.h"

namespace nntp
{
    // request the server for the current list of capabilities. 
    // this mechanism includes some standard capabilities as well as server
    // specific extensions. upon receiving a response we scan
    // the list of returned capabilities and set the appropriate boolean flags
    // to indicate the existence of said capability.
    // it seems that some servers such as astraweb respond to this with 500
    // 101 capability list follows.
    // 480 authentication required
    // 500 what? 
    struct cmd_capabilities : cmd {

        enum : code_t { SUCCESS = 101 };

        bool has_mode_reader;
        bool has_compress_gzip;
        bool has_xzver;

        cmd_capabilities();

        code_t transact();
    };

} // nntp
