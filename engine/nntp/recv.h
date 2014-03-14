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

#include <utility> // for pair
#include <string>
#include "cmd.h"
#include "exception.h"

namespace nntp
{
    // receive nntp data into the given buffer untill the test predicate evaluates to true.
    // returns the message length and the total number of bytes received.
    template<typename Pred>
    std::pair<std::size_t, std::size_t> recv(const cmd& cmd, Pred test, void* buff, std::size_t buffsize)
    {
        std::size_t total_bytes_received = 0;
        std::size_t message_length = 0;
        do 
        {
            const std::size_t capacity = buffsize - total_bytes_received;
            const std::size_t bytes = cmd.recv(((char*)buff) + total_bytes_received, capacity);
            if (bytes == 0)
                throw nntp::exception("no input");

            total_bytes_received += bytes;
            message_length = test(buff, total_bytes_received);
        } 
        while (!message_length && (total_bytes_received < buffsize));

        return { message_length, total_bytes_received };
    }

    // convenience wrapper for receiving nntp data into a std::string
    template<typename Pred>
    std::pair<std::size_t, std::size_t> recv(const cmd& cmd, Pred test, std::string& buff)
    {
        return recv(cmd, test, &buff[0], buff.size());
    }

} // nntp
