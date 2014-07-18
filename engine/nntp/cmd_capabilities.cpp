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

#include "cmd_capabilities.h"
#include "transact.h"
#include "nntp.h"
#include "buffer.h"

namespace nntp
{

cmd_capabilities::cmd_capabilities() : has_mode_reader(false), has_compress_gzip(false), has_xzver(false)
{}

code_t cmd_capabilities::transact()
{
    std::string response(1024, 0);
    std::size_t offset = 0;
    std::size_t size   = 0;
    code_t code = 0;

    std::tie(code, offset, size) = nntp::transact(*this, "CAPABILITIES", {101, 480, 500}, {101}, response);
    if (code == SUCCESS)
    {
        const auto& lines = split_lines(response);
        for (size_t i=1; i<lines.size(); ++i)
        {
            const auto& cap = lines[i];
            if (cap.find("MODE-READER") != std::string::npos)
                has_mode_reader = true;
            else if (cap.find("XZVER") != std::string::npos)
                has_xzver = true;
            else if (cap.find("COMPRESS") != std::string::npos && cap.find("GZIP") != std::string::npos)
                has_compress_gzip = true;
        }
    }
    return code;
}

} // nntp
