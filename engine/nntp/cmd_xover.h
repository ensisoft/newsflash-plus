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
    // XOVER is part of common nntp extensions
    // https://www.ietf.org/rfc/rfc2980.txt 
    // and is superceded by RFC 3977 and OVER
    // 224 overview information follows
    // 412 no news group selected
    // 420 no articles(s) selected
    // 502 no permission    
    template<typename Buffer>
    struct cmd_xover : cmd
    {
        enum : code_t { SUCCESS = 224 };            
    
        Buffer& buffer;
        std::string first;
        std::string last;
        std::size_t offset;
        std::size_t size;

        cmd_xover(Buffer& buff, std::string start, std::string end) : buffer(buff),
             first(std::move(start)), last(std::move(end)),
             offset(0), size(0)
        {}

        code_t transact()
        {
            code_t status = 0;
            std::tie(status, offset, size) = nntp::transact(*this, "XOVER " + first + "-" + last, {224, 412, 420, 502}, {224}, buffer);                
            return status;
        }
    };
 
} // nntp
