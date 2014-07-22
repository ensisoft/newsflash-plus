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
#include "session.h"
#include "nntp.h"

namespace nntp
{

const int AUTHENTICATION_REQUIRED = 480;


class session::command
{

};

class session::state 
{
public:
    virtual ~state() = default;
    virtual session::status check() const = 0;
    virtual void write(std::string& str) = 0;
    virtual int read(response& buff) = 0;
protected:
private:
};

// in this state we expect to receive the initial greeting from the server.
class session::read_welcome : public session::state
{
public:
    read_welcome() : status_(status::want_read)
    {}
    virtual session::status check() const override
    { 
        return status_;
    }

    virtual int read(response& buff) override
    {
        int status = 0;
        const auto len = nntp::find_response(buff.data(),
            buff.size());
        if (len == 0)
            return status;
        const std::string greeting(buff.data(), len-2);
        if (!nntp::scan_response(greeting, status) ||
            !nntp::find_status(status, {200, 201, 400, 502}))
            throw nntp::exception("incorrect greeting from the server");

        if (status == 400)
        {
            status_ = session::status::error;
            error_  = error::service_temporarily_unavailable;
        }
        else if (status == 502)
        {
            status_ = session::status::error;
            error_  = error::service_permanently_unavailable;
        }
        else 
        {
            status_ = session::status::none;
        }
        return status;
    }

    virtual void write(std::string& str) override
    {}
private:
    session::status status_;
    session::error error_;

};

// send MODE READER
class session::send_mode_reader : public session::state
{
public:
    send_mode_reader()
    {}

    virtual session::status check() const override
    {

    }
private:
    
};

session::session()
{}

session::~session()
{}

// session::status session::establish()
// {
//     has_compress_gzip_ = false;
//     has_xzver_ = false;
//     has_mode_reader_ = false;

//     // buff_.clear();
//     // buff_.allocate(1024 * 1024);
//     // sess_.clear();
//     // sess_.emplace_back(new cmd_welcome);
//     // sess_.emplace_back(new cmd_capabilities);

//     return status::read;
// }

session::status session::check()
{
    return status::none;
}

session::error session::err() const
{
    return error::none;
}

void session::write()
{
    std::string str;
    state_->write(str);
    if (str.empty())
        return;

    on_send(&str[0], str.size());
}

void session::read()
{
    auto capacity  = buff_.capacity();
    auto consumed  = buff_.size();
    auto available = capacity - consumed;
    if (!available)
    {
        capacity = std::max(size_t(1024 * 1024), capacity * 2);
        if (capacity >= 1024 * 1024 * 4)
            throw nntp::exception("response buffer maximum size exceeded");

        buff_.allocate(capacity);
        available = capacity - consumed;
    }

    const auto ret = on_recv(buff_.data() + consumed, available);
    if (ret == 0)
        return;

    consumed  += ret;
    available -= ret;
    buff_.resize(consumed);

    const auto status = state_->read(buff_);

    if (status == AUTHENTICATION_REQUIRED)
    {

    }
}

} // nntp
