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

#include <newsflash/config.h>

#include "session.h"
#include "buffer.h"
#include "nntp.h"

namespace newsflash
{

struct session::state {
    capabilities caps;
    session::error err;
    bool authentication_required;
};

class session::command 
{
public:
    enum class type {
        welcome, capabilities, authuser, authpass, modereader
    };

    virtual ~command() = default;
    virtual bool inspect(buffer& buff, state& st) const = 0; 
    virtual type identity() const = 0;
protected:
};

// read initial greeting from the server
class session::welcome : public session::command
{
public:
    virtual bool inspect(buffer& buff, session::state& st) const override
    {
        const auto len = nntp::find_response(buff.head(), buff.size());
        if (len == 0)
            return false;

        const auto code = nntp::scan_response({200, 201, 400, 502}, buff.head(), len-2);
        if (code == 400)
            st.err = error::service_temporarily_unavailable;
        else if (code == 502)
            st.err = error::service_permanently_unavailable;

        buff.set_header_length(len);
        return true;
    }
    virtual type identity() const override
    { return type::welcome; }
public:
};

class session::getcaps : public session::command
{
public:
    virtual bool inspect(buffer& buff, session::state& st) const override
    {
        const auto res = nntp::find_response(buff.head(), buff.size());
        if (res == 0)
            return false;

        const auto code = nntp::scan_response({101, 480, 500}, buff.head(), res-2);
        if (code == 500)
            return true; // we're done, they don't understand
        else if (code == 480) {
            st.authentication_required = true;
            return true;
        }

        const auto ptr = buff.head() + res;
        const auto len = nntp::find_body(ptr, buff.size() - res);
        if (len == 0)
            return false;

        std::string caps(ptr, len);
        std::stringstream ss(caps);
        std::string line;
        while (std::getline(ss, line))
        {
            if (line.find("MODE-READER") != std::string::npos)
                st.caps.modereader = true;
            else if (line.find("XZVER") != std::string::npos)
                st.caps.xzver = true;
            else if (line.find("COMPRESS") != std::string::npos &&
                line.find("GZIP") != std::string::npos)
                st.caps.gzip = true;
        }
        buff.set_header_length(res);
        buff.set_body_length(len);
        return true;
    }
    virtual type identity() const override
    { return type::capabilities; }
};

class session::authuser : public session::command
{
public:
    virtual bool inspect(buffer& buff, state& st)const override
    {}
    virtual type identity() const 
    { return type::authuser; }
private:
};

class session::authpass : public session::command
{
public:
    virtual bool inspect(buffer& buff, state& st) const override
    { }
    virtual type identity() const 
    { return type::authpass; }
};

class session::modereader : public session::command
{
public:
    virtual bool inspect(buffer& buff, state& st) const override
    {}
    virtual type identity() const 
    { return type::modereader; }
};

session::session()
{}

session::~session()
{}

void session::start()
{
    command_.reset(new welcome);
}

bool session::inspect(buffer& buff)
{
    bool ret = command_->inspect(buff, *state_);
    if (ret)
    {
        if (state_->authentication_required)
        {
            needs_authentication_ = std::move(command_);
            std::string username;
            std::string password;
            on_auth(username, password);
            command_.reset(new authuser);
            on_send("AUTHINFO USER " + username + "\r\n");
            return true;
        }

        switch (command_->identity())
        {
            case command::type::welcome:
                on_send("CAPABILITIES\r\n");
                command_.reset(new getcaps);
                break;
            case command::type::capabilities:
                if (state_->caps.modereader)
                {
                    on_send("MODE READER\r\n");
                    command_.reset(new modereader);
                }
                break;
        }
    }
    return ret;
}

} // newsflash
