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
#include <sstream>
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
    state() : status_(status::want_write), error_(error::none)
    {}

    virtual ~state() = default;

    virtual void write(std::string& str) = 0;
    virtual int read(response& buff) = 0;

    status check() const 
    { return status_; }

    error err() const
    { return error_; }

protected:
    status status_;
    error  error_;
private:
};



// in this state we expect to receive the initial greeting from the server.
// 200 service available, posting allowed
// 201 service available, posting prohibited
// 400 service temporarily unavailable
// 502 service permanently unavailable
class session::welcome : public session::state
{
public:
    welcome()
    {
        status_ = status::want_read;
    }
    virtual void write(std::string& str) override
    {}

    virtual int read(response& buff) override
    {
        int code = 0;
        const auto len = nntp::find_response(buff.data(),
            buff.size());
        if (len == 0)
            return code;

        const std::string greeting(buff.data(), len-2);

        if (!nntp::scan_response(greeting, code) ||
            !nntp::find_status(code, {200, 201, 400, 502}))
            throw nntp::exception("incorrect greeting from the server");

        if (code == 400)
        {
            status_ = status::error;
            error_  = error::service_temporarily_unavailable;
        }
        else if (code == 502)
        {
            status_ = status::error;
            error_  = error::service_permanently_unavailable;
        }
        else 
        {
            status_ = status::none;
        }
        buff.clear();
        return code;
    }
private:
};

// request the server for the current list of capabilities
// this mechanism includes some standard capabilities 
// as well as server specific extensions
// NOTE: some servers (astraweb) responds to this with 500  what?
// 101 capability list follows
// 480 authentication required
// 500 what?
class session::capabilities : public session::state
{
public:
    bool has_compress_gzip;
    bool has_xzver;
    bool has_mode_reader;

    capabilities() : has_compress_gzip(false), 
        has_xzver(false), has_mode_reader(false)
        {}

    virtual void write(std::string& str) override
    {
        str = "CAPABILITIES\r\n";
    }
    virtual int read(response& buff) override
    {
        int code = 0;
        auto len = nntp::find_response(buff.data(), 
            buff.size());
        if (len == 0)
            return code;

        const std::string str(buff.data(), len-2);

        if (!nntp::scan_response(str, code) ||
            !nntp::find_status(code, {101, 480, 500}))
            throw nntp::exception("incorrect CAPABILITIES response");

        if (code == 480)
            return code;
        else if (code == 500)
        {
            status_ = status::none;
            return code;
        }

        const auto ptr  = buff.data() + len;
        const auto size = buff.size() - len;

        len = nntp::find_body(ptr, size);
        if (len == 0)
            return 0;

        const std::string caps(ptr, len-3);

        const auto& lines = split(caps);
        for (std::size_t i=1; i<lines.size(); ++i)
        {
            const auto& cap = lines[i];
            if (cap.find("MODE READER") != std::string::npos)
                has_mode_reader = true;
            else if (cap.find("XZVER") != std::string::npos)
                has_xzver = true;
            else if (cap.find("COMPRESS") != std::string::npos &&
                cap.find("GZIP") != std::string::npos)
                has_compress_gzip = true;
        }

        buff.clear();
        return code;
    }
private:
    std::deque<std::string> split(const std::string& caps) const
    {
        std::deque<std::string> ret;
        std::stringstream ss(caps);
        std::string line;
        while (!ss.eof())
        {
            std::getline(ss, line);
            ret.push_back(line);
        }
        return ret;
    }
};

// send MODE READER
// 200 posting allowed
// 201 posting prohibited
// 502 reading service permanently unavailable
class session::modereader : public session::state
{
public:
    virtual void write(std::string& str) override
    {
        str = "MODE READER\r\n";
    }
    virtual int read(response& buff) override
    {
        int code = 0;
        const auto len = nntp::find_response(buff.data(),
            buff.size());
        if (len == 0)
            return code;

        const std::string str(buff.data(), len-2);

        if (!nntp::scan_response(str, code) || 
            !nntp::find_status(code, {200, 201, 502}))
            throw nntp::exception("incorrect MODE READER response");

        if (code == 502)
        {
            status_ = status::error;
            error_  = error::service_temporarily_unavailable;
        }
        else 
        {
            status_ = status::none;
            error_  = error::none;
        }
        buff.clear();
        return code;
    }
private:
};

class session::authenticate : public session::state
{
public:
    virtual void write(std::string& str) override
    {}

    virtual int read(response& buff) override
    {
        int code = 0;

        return code;
    }
private:
};

session::session()
{}

session::~session()
{}

session::status session::check()
{
    if (!state_)
        return status::none;

    const auto ret = state_->check();
    if (ret == status::error)
        return ret;

    if (auto ptr = dynamic_cast<welcome*>(state_.get()))
    {
        state_.reset(new capabilities);
    }
    else if (auto ptr = dynamic_cast<capabilities*>(state_.get()))
    {
        if (ptr->has_mode_reader)
        {
            state_.reset(new modereader);
        }
        else
        {}
    }
    else if (auto ptr = dynamic_cast<modereader*>(state_.get()))
    {

    }
    else if  (auto ptr = dynamic_cast<authenticate*>(state_.get()))
    {

    }


    return state_->check();
}

session::error session::err() const
{
    if (state_)
        return state_->err();
    return error::none;
}

void session::start()
{
    state_.reset(new welcome);
    has_compress_gzip_ = false;
    has_xzver_ = false;
    has_mode_reader_ = false;
    buff_.clear();
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
