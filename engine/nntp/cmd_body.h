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
#include "nntp.h"
#include "buffer.h"

namespace nntp
{
    // request to receive the body of the article identified
    // by article. note that this can either be the message number
    // or the message-id in angle brackets ("<" and ">")
    // 222 body follows
    // 420 no article with that message-id
    // 423 no article with that number
    // 412 no newsgroup selected
    template<typename Buffer>
    struct cmd_body : cmd {

        enum : code_t { SUCCESS = 222 };        

        Buffer& buffer;
        std::string article;        
        std::string reason;        
        std::size_t offset;
        std::size_t size;

        cmd_body(Buffer& buff, std::string id) : buffer(buff), article(std::move(id)),
            offset(0), size(0)
        {}

        code_t transact()
        {
            code_t status = 0;
            std::tie(status, offset, size) = nntp::transact(*this, "BODY " + article, {222, 420, 423, 412}, {222}, buffer);
            if (status == SUCCESS)
                return status;

            // store the comment after the status code
            // it might give us a hint why the article was no longer
            // available (dmca takedown for example)
            const auto data = static_cast<const char*>(buffer_data(buffer));
            const auto size = buffer_capacity(buffer);
            const auto len  = find_response(data, size);
            const std::string response(data, len);

            trailing_comment comment;
            scan_response(response, status, comment);

            reason = std::move(comment.str);
            return status;
        }
    };

} // nntp

