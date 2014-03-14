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

#include "cmd_group.h"
#include "send.h"
#include "recv.h"
#include "nntp.h"
#include "scan.h"

namespace nntp
{

cmd_group::cmd_group(std::string groupname) : 
    group(std::move(groupname)), count(0), low(0), high(0)
{}

code_t cmd_group::transact()
{
    nntp::send(*this, "GROUP " + group);

    std::string response(512, 0);

    const auto& ret = nntp::recv(*this, find_response, response);
    if (!ret.first)
        throw nntp::exception("no response found");

    response.resize(ret.first-2);

    if (log)
        log(response);

    code_t code = 0;
    const bool success = scan_response(response, code, count, low, high);
    if (((code != 211) && (code != 411)) || (code == 211 && !success))
        throw exception("incorrect command response");

    return code;
}

} // nntp
