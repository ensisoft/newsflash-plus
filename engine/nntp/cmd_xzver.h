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

#include <zlib/zlib.h>
#include <vector>
#include "cmd.h"
#include "transact.h"
#include "exception.h"
#include "buffer.h"
#include "nntp.h"
#include "scan.h"

namespace nntp
{
    // like XOVER except that compressed.
    template<typename Buffer>
    struct cmd_xzver : cmd
    {
        enum : code_t { SUCCESS = 224 };

        Buffer& buffer;
        std::string first;
        std::string last;
        std::size_t offset;
        std::size_t size;        

        cmd_xzver(Buffer& buff, std::string start, std::string end) : buffer(buff),
           first(std::move(start)), last(std::move(end)),
           offset(0), size(0)
        {}

        code_t transact()
        {
            std::vector<char> body(1024 * 1024);

            const auto& ret = nntp::recv(*this, find_response, buffer_data(body), buffer_capacity(body));
            if (!ret.second)
                throw nntp::exception("no response was found within the buffer");

            const std::string response((char*)buffer_data(body), ret.first);
            code_t status = 0;
            if (!scan_response(response, status) ||!check_code(status, {224, 412, 420, 502}))
                throw nntp::exception("incorrect command response");

            const auto body_offset = ret.first;
            const auto data_total  = ret.second;

            struct Z {
                z_stream z;
                Z() {
                    z = z_stream {0};
                }

               ~Z() {
                    inflateEnd(&z);
                }
            } zstream;

            // begin body decompression, read data utill z inflate is happy.
            zstream.z.avail_in = data_total - body_offset;
            zstream.z.next_in  = (Bytef*)buffer_data(body);
            inflateInit(&zstream.z);

            // uLong out = 0;
            // uLong in  = 0;

            int err = Z_OK;
            do
            {
                zstream.z.next_out = (Bytef*)buffer_data(buffer) + size;
                zstream.z.avail_out = buffer_capacity(buffer) - size;
                zstream.z.next_in = (Bytef*)buffer_data(body);
                zstream.z.avail_in  = 0;
                // todo:
            }
            while (err != Z_STREAM_END);

            return status;
        }
    };

} // nntp
