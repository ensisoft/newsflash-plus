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

struct session::impl {
    bool has_gzip;
    bool has_xzver;
    bool has_modereader;
    bool auth_user_required;
    bool auth_pass_required;
    std::string user;
    std::string pass;
    session::error error;
    session::state state;
};

class session::command
{
public:
    enum class type {
        welcome, capabilities, authuser, authpass, modereader
    };

    virtual ~command() = default;

    virtual bool execute(buffer& buff, impl& st) const = 0;

    virtual type identity() const = 0;

    virtual std::string str() const = 0;
protected:
private:
};

// read initial greeting from the server
class session::welcome : public session::command
{
public:
    virtual bool execute(buffer& buff, session::impl& st) const override
    {
        const auto len = nntp::find_response(buff.head(), buff.size());
        if (len == 0)
            return false;

        const auto code = nntp::scan_response({200, 201, 400, 502}, buff.head(), len-2);
        if (code == 400)
            st.error = error::service_temporarily_unavailable;
        else if (code == 502)
            st.error = error::service_permanently_unavailable;

        buff.set_header_length(len);
        return true;
    }
    virtual type identity() const override
    { return type::welcome; }

    virtual std::string str() const 
    { return ""; }
private:
};

// query server for supported capabilities
class session::getcaps : public session::command
{
public:
    virtual bool execute(buffer& buff, session::impl& st) const override
    {
        const auto res = nntp::find_response(buff.head(), buff.size());
        if (res == 0)
            return false;

        const auto code = nntp::scan_response({101, 480, 500}, buff.head(), res-2);
        if (code != 101)
        {
            if (code == 480)
                st.auth_user_required = true;
            buff.set_header_length(res);
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
                st.has_modereader = true;
            else if (line.find("XZVER") != std::string::npos)
                st.has_xzver = true;
            else if (line.find("COMPRESS") != std::string::npos &&
                line.find("GZIP") != std::string::npos)
                st.has_gzip = true;
        }
        buff.set_header_length(res);
        buff.set_body_length(len);
        return true;
    }
    virtual type identity() const override
    { return type::capabilities; }

    virtual std::string str() const override
    { return "CAPABILITIES\r\n"; }
private:
};

// perform user authentication 
class session::authuser : public session::command
{
public:
    authuser(std::string username) : username_(std::move(username))
    {}

    virtual bool execute(buffer& buff, impl& st) const override
    {
        const auto len = nntp::find_response(buff.head(), buff.size());
        if (len == 0)
            return false;

        // we also have 482 and 502 responses in the spec but
        // consider these to be a "programmer error"
        // 482 authentication command issued out of sequence
        // 502 command unavailable

        const auto code = nntp::scan_response({281, 381, 482, 502}, buff.head(), len-2);
        if (code == 381)
            st.auth_pass_required = true;
        else if (code == 482)
            st.error = error::authentication_rejected;
        else if (code == 502)
            st.error = error::no_permission;

        buff.set_header_length(len);
        return true;
    }
    virtual type identity() const 
    { return type::authuser; }

    virtual std::string str() const 
    { return "AUTHINFO USER " + username_ + "\r\n"; }
private:
    std::string username_;
};


class session::authpass : public session::command
{
public:
    authpass(std::string password) : password_(std::move(password))
    {}

    virtual bool execute(buffer& buff, impl& st) const override
    { 
        const auto len = nntp::find_response(buff.head(), buff.size());
        if (len == 0)
            return false;

        const auto code = nntp::scan_response({281, 482, 502}, buff.head(), len-2);
        if (code == 482)
            st.error = error::authentication_rejected;
        else if (code == 502)
            st.error = error::no_permission;

        buff.set_header_length(len);
        return true;
    }
    virtual type identity() const 
    { return type::authpass; }

    virtual std::string str() const
    { return "AUTHINFO PASS " + password_ + "\r\n"; }
private:
    std::string password_;
};


class session::modereader : public session::command
{
public:
    virtual bool execute(buffer& buff, impl& st) const override
    {
        const auto len = nntp::find_response(buff.head(), buff.size());
        if (len == 0)
            return false;
        const auto code = nntp::scan_response({200, 201, 480}, buff.head(), len-2);
        if (code == 480)
            st.auth_user_required = true;

        buff.set_header_length(len);
        return true;
    }
    virtual type identity() const 
    { return type::modereader; }

    virtual std::string str() const 
    { return "MODE READER\r\n"; }
private:
};


session::session() : state_(new impl)
{
    reset();
}

session::~session()
{}

void session::reset()
{
    current_.reset(new welcome);
    state_->auth_user_required = false;
    state_->auth_pass_required = false;
    state_->has_gzip           = false;
    state_->has_modereader     = false;
    state_->has_xzver          = false;
    state_->error              = error::none;
    state_->state              = state::none;
}

bool session::initialize(buffer& buff)
{
    if (state_->state == state::none)
        state_->state = state::init;

    if (!current_->execute(buff, *state_))
        return false;

    if (state_->error != error::none)
    {
        state_->state = state::error;
        current_.reset();
        retry_.reset();
    }
    else if (state_->auth_user_required)
    {
        retry_ = std::move(current_);

        std::string username;
        std::string password;
        on_auth(username, password);
        current_.reset(new authuser(username));
        state_->auth_user_required = false;
        state_->state = state::authenticate;
    }
    else if (state_->auth_pass_required)
    {
        std::string username;
        std::string password;
        on_auth(username, password);
        current_.reset(new authpass(password));
        state_->auth_pass_required = false;
        state_->state = state::authenticate;        
    }
    else
    {
        const auto type = current_->identity();
        current_.reset();
        switch (type)
        {
            case command::type::welcome:
                current_.reset(new getcaps);
                break;

            case command::type::capabilities:
                current_.reset(new modereader);
                break;

            case command::type::authuser:
            case command::type::authpass:
                current_ = std::move(retry_);
                break;

            case command::type::modereader:
                break;
        }
        state_->state = state::init;                        
    }
    if (current_)
    {
        const auto str = current_->str();
        if (!str.empty())
            on_send(current_->str());            
    }
    else if (state_->error == error::none)
        state_->state = state::ready;

    return !current_;

}

session::error session::get_error() const
{ return state_->error; }

session::state session::get_state() const
{ return state_->state; }

bool session::has_xzver() const 
{ return state_->has_xzver; }

bool session::has_gzip_compress() const
{ return state_->has_gzip; }

} // newsflash
