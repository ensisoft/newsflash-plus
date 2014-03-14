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

#include <tuple>
#include <cassert>
#include "cmd.h"
#include "send.h"
#include "recv.h"
#include "nntp.h"
#include "scan.h"

namespace nntp
{
    // perform a send/recv transaction. no body is returned.
    code_t transact(const cmd& cmd, const std::string& str, const code_list_t& allowed_codes);

    typedef std::tuple<code_t, std::size_t, std::size_t> tuple_t;

    // perform send/recv transaction. read the response body into the body Buffer
    // returns a tuple of response code, message size, body size.
    template<typename Buffer>
    tuple_t transact(const cmd& cmd, const std::string& str, const code_list_t& allowed_codes, const code_list_t& success_codes, Buffer& body)
    {
        send(cmd, str);

        const auto& ret = recv(cmd, find_response, buffer_data(body), buffer_capacity(body));
        if (!ret.first)
            throw nntp::exception("no response was found within the response buffer");

        const std::string response((char*)buffer_data(body), ret.first-2);

        if (cmd.log)
            cmd.log(response);

        code_t status = 0;
        if (!scan_response(response, status) | !check_code(status, allowed_codes))
            throw nntp::exception("incorrect command response");

        // if response code is not succesful, then there's no
        // body to be received. so we can return now.
        if (!check_code(status, success_codes))
            return std::make_tuple(status, 0, 0);

        assert(ret.first <= ret.second);

        std::size_t data_total  = ret.second;
        std::size_t body_offset = ret.first; 

        // get a pointer to the start of the buffer
        // and skip the response header. 
        char* body_ptr = static_cast<char*>(buffer_data(body));

        // check if we've already have the full body in the buffer
        std::size_t body_len = find_body(body_ptr + body_offset, data_total - body_offset);

        while (!body_len)
        {
            const std::size_t capacity = buffer_capacity(body);
            std::size_t available = capacity - data_total;
            if (!available)
            {
                available = capacity;
                grow_buffer(body, capacity * 2);
                body_ptr = static_cast<char*>(buffer_data(body));
            }
            const auto& ret = recv(cmd, find_body, body_ptr + data_total, available);
            data_total += ret.second;
            body_len    = ret.first;
        }

        assert(body_len >= 5);
        assert(data_total >= body_len);
            
        return std::make_tuple(status, body_offset, data_total - body_offset - 5);  // omit \r\n.\r\n from the body length
    }

} // nntp