// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
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

#include <newsflash/config.h>
#include "datalink.h"
#include "tcpsocket.h"
#include "sslsocket.h"
#include "command.h"
#include "buffer.h"

namespace newsflash
{

datalink::datalink(ipv4addr_t host, port_t port, bool ssl)
{
    if (ssl)
        socket_.reset(new sslsocket());
    else socket_.reset(new tcpsocket());
    socket_->begin_connect(host, port);
}

datalink::~datalink()
{}

void datalink::send(const command* cmd) 
{
    std::string str;
    cmd->write(str);
    socket_->sendall(&str[0], str.size());
}

bool datalink::recv(const command* cmd, buffer& buff) 
{
    auto space = buffer_.capacity() - buffer_.size();
    if (!space)
    {
        const auto capacity = std::max(1024 * 1024, 
            (int)buffer_.capacity() * 2);
        if (capacity >= 1024 * 1024 * 4)
            throw std::runtime_error("buffer growing too large");
        buffer_.reserve(capacity);
        space = buffer_.capacity() - buffer_.size();
    }
    auto size  = buffer_.size();
    auto* data = buffer_.data();
    data += size;

    const auto ret = socket_->recvsome(data, space);

    buffer_.resize(size + ret);

    return cmd->extract(buffer_, buff);
}

waithandle datalink::wait(bool send, bool recv) const
{
    return socket_->wait(recv, send);
}

} // newsflash
