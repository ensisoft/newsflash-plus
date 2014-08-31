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

#include <zlib/zlib.h>

#include "session.h"
#include "buffer.h"
#include "nntp.h"
#include "logging.h"
#include "format.h"

namespace newsflash
{

struct session::impl {
    bool has_gzip;
    bool has_xzver;
    bool has_modereader;
    bool auth_required;
    std::string user;
    std::string pass;
    std::string group;
    session::error error;
    session::state state;
};

// command encapsulates a request/response transaction.
class session::command
{
public:
    virtual ~command() = default;

    // parse the input buffer and store the possible data contents
    // into output buffer.
    virtual bool parse(buffer& buff, buffer& out, impl& st) = 0;

    // get the state represented by this command
    virtual session::state state() const = 0;

    // get the nntp command string
    virtual std::string str() const = 0;
protected:
private:
};


// read initial greeting from the server
class session::welcome : public session::command
{
public:
    virtual bool parse(buffer& buff, buffer& out, session::impl& st) override
    {
        const auto len = nntp::find_response(buff.head(), buff.size());
        if (len == 0)
            return false;

        // i have never seen these actually being used so we're just 
        // going to pretend they dont exist.
        // 400 service temporarily unavailable
        // 502 service permantly unavailable
        nntp::scan_response({200, 201}, buff.head(), len);
        buff.clear();
        return true;
    }
    virtual session::state state() const override
    { return session::state::init; }

    virtual std::string str() const override
    { return ""; }
private:
};

// query server for supported capabilities
class session::getcaps : public session::command
{
public:
    virtual bool parse(buffer& buff, buffer& out, session::impl& st) override
    {
        const auto res = nntp::find_response(buff.head(), buff.size());
        if (res == 0)
            return false;

        const auto code = nntp::scan_response({101, 480, 500}, buff.head(), res);
        if (code != 101)
        {
            if (code == 480)
                st.auth_required = true;
            buff.clear();
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
        buff.clear();
        return true;
    }
    virtual session::state state() const override
    { return session::state::init; }

    virtual std::string str() const override
    { return "CAPABILITIES"; }
private:
};

// perform user authentication (username)
class session::authuser : public session::command
{
public:
    authuser(std::string username) : username_(std::move(username))
    {}

    virtual bool parse(buffer& buff, buffer& out, impl& st) override
    {
        const auto len = nntp::find_response(buff.head(), buff.size());
        if (len == 0)
            return false;

        // we also have 482 and 502 responses in the spec but
        // consider these to be a "programmer error"
        // 482 authentication command issued out of sequence
        // 502 command unavailable

        const auto code = nntp::scan_response({281, 381, 482, 502}, buff.head(), len);
        if (code == 482)
            st.error = error::authentication_rejected;
        else if (code == 502)
            st.error = error::no_permission;

        buff.clear();
        return true;
    }
    virtual session::state state() const override
    { return session::state::authenticate; }

    virtual std::string str() const override
    { return "AUTHINFO USER " + username_; }
private:
    std::string username_;
};

// perform user authenticaton (password)
class session::authpass : public session::command
{
public:
    authpass(std::string password) : password_(std::move(password))
    {}

    virtual bool parse(buffer& buff, buffer& out, impl& st) override
    {
        const auto len = nntp::find_response(buff.head(), buff.size());
        if ( len== 0)
            return false;

        const auto code = nntp::scan_response({281, 482, 502}, buff.head(), len);
        if (code == 482)
            st.error = error::authentication_rejected;
        else if (code == 502)
            st.error = error::no_permission;

        buff.clear();
        return true;
    }
    virtual session::state state() const override
    { return session::state::authenticate; }

    virtual std::string str() const override
    { return "AUTHINFO PASS " + password_; }
private:
    std::string password_;
};

// set mode reader for server
class session::modereader : public session::command
{
public:
    virtual bool parse(buffer& buff, buffer& out, impl& st) override
    {
        const auto len = nntp::find_response(buff.head(), buff.size());
        if (len == 0)
            return false;
        const auto code = nntp::scan_response({200, 201, 480}, buff.head(), len);
        if (code == 480)
            st.auth_required = true;

        buff.clear();
        return true;
    }
    virtual session::state state() const override
    { return session::state::init; }

    virtual std::string str() const override
    { return "MODE READER"; }
private:
};

// change group/store group information into data buffer
class session::group : public session::command
{
public:
    group(std::string name) : group_(std::move(name)), count_(0), low_(0), high_(0)
    {}

    virtual bool parse(buffer& buff, buffer& out, impl& st) override
    {
        const auto len = nntp::find_response(buff.head(), buff.size());
        if (len == 0)
            return false;

        const auto code = nntp::scan_response({211, 411}, buff.head(), len);
        if (code == 411)
        {
            out.set_status(buffer::status::unavailable);
        }
        else
        {
            out = buff.split(len);
            out.set_content_length(len);
            out.set_content_start(0);                        
            out.set_status(buffer::status::success);
        }
        out.set_content_type(buffer::type::groupinfo);
        return true;
    }

    virtual session::state state() const override
    {
        return session::state::transfer;
    }


    virtual std::string str() const override
    { 
        return "GROUP " + group_; 
    }
private:
    std::string group_;
private:
    std::size_t count_;
    std::size_t low_;
    std::size_t high_;
};

// request article data from the server. store into output buffer
class session::body : public session::command
{
public:
    body(std::string messageid) : messageid_(std::move(messageid))
    {}

    virtual bool parse(buffer& buff, buffer& out, impl& st) override
    {
        const auto len = nntp::find_response(buff.head(), buff.size());
        if (len == 0)
            return false;

        out.set_content_type(buffer::type::article);

        nntp::trailing_comment comment;
        const auto code = nntp::scan_response({222, 423, 420}, buff.head(), len, comment);
        if (code == 423 || code == 420)
        {
            if (comment.str.find("dmca") != std::string::npos)
                out.set_status(buffer::status::dmca);
            else out.set_status(buffer::status::unavailable);
            buff.split(len);
            return true;
        }

        const auto blen = nntp::find_body(buff.head() + len, buff.size() - len);
        if (blen == 0)
            return false;

        const auto size = len + blen;
        out = buff.split(size);
        out.set_content_length(blen);
        out.set_content_start(len);
        out.set_status(buffer::status::success);
        return true;
    }

    virtual session::state state() const override
    { 
        return session::state::transfer; 
    }

    virtual std::string str() const override
    { 
        return "BODY " + messageid_; 
    }
private:
    std::string messageid_;
};


// quit session.
class session::quit : public session::command
{
public:

    virtual bool parse(buffer& buff, buffer& out, impl& st) override
    {
        const auto len = nntp::find_response(buff.head(), buff.size());
        if (len == 0)
            return false;

        nntp::scan_response({205}, buff.head(), len);
        return true;
    }

    virtual session::state state() const override
    { 
        return session::state::quitting;
    }

    virtual std::string str() const override
    { 
        return "QUIT"; 
    }

private:

};

class session::xover : public session::command
{

};

// like xover but libz compressed
class session::xzver : public session::command
{
public:
    xzver(std::string first, std::string last) : first_(std::move(first)), last_(std::move(last))
    {}

    virtual bool parse(buffer& buff, buffer& out, impl& st) override
    {
        const auto len = nntp::find_response(buff.head(), buff.size());
        if (len == 0)
            return false;

        // also 
        // 412 no news group selected
        // 420 no article(s) selected
        // 502 no permssion 
        nntp::scan_response({224}, buff.head(), len);

        const auto blen = nntp::find_body(buff.head() + len, buff.size() - len);
        if (blen == 0)
            return false;


        out.set_content_type(buffer::type::overview);
        out.allocate(blen * 2);

        z_stream z;
        inflateInit(&z);

        const auto* inptr = buff.head() + len;
        const auto avail  = buff.size() - len;

        uLong obytes = 0; 
        uLong ibytes = 0;
        int err = Z_OK;
        for (;;)
        {
            obytes = z.total_out;
            ibytes = z.total_in;
            z.next_out  = (Bytef*)out.back();
            z.avail_out = out.available();
            z.next_in   = (Bytef*)inptr + ibytes;
            z.avail_in  = avail - ibytes;
            err = inflate(&z, Z_NO_FLUSH);
            if (err == Z_STREAM_END)
                break;
            else if(err == Z_BUF_ERROR)
                out.allocate(obytes * 2);
            else if (err < 0)
                break;
        }
        inflateEnd(&z);

        if (err != Z_STREAM_END)
        {
            LOG_E("Inflate failed zlib error: ", err);        
            out.set_status(buffer::status::error);
        }
        else
        {
            LOG_I("Inflated header data from ", kb(ibytes), " to ", kb(obytes));

            out.set_status(buffer::status::success);
            out.set_content_start(0);
            out.set_content_length(obytes);
        }
        return true;
    }
    virtual session::state state() const override
    { return session::state::transfer; }

    virtual std::string str() const 
    { return "XOVER " + first_ + "-" + last_; }
    
private:
    std::string first_;
    std::string last_;
};


session::session() : state_(new impl)
{
    reset();
}

session::~session()
{}

void session::reset()
{
    pipeline_.clear();
    state_->auth_required  = false;
    state_->has_gzip       = false;
    state_->has_modereader = false;
    state_->has_xzver      = false;
    state_->error          = error::none;
    state_->state          = state::none;
    state_->group          = "";
}

void session::start()
{
    pipeline_.emplace_back(new welcome);
    pipeline_.emplace_back(new getcaps);
    pipeline_.emplace_back(new modereader);
}

void session::quit()
{
    pipeline_.emplace_back(new class quit);
}

bool session::parse_next(buffer& buff, buffer& out)
{
    if (state_->state == state::none)
        state_->state = state::init;

    assert(!pipeline_.empty());

    command* current = pipeline_.front().get();

    const auto len = nntp::find_response(buff.head(), buff.size());
    if (len != 0)
    {
        LOG_I(std::string(buff.head(), len-2));
    }

    if (!current->parse(buff, out, *state_))
        return false;

    if (state_->error != error::none)
    {
        state_->state = state::error;
        pipeline_.clear();
        return true;
    }
    else if (state_->auth_required)
    {
        std::string username;
        std::string password;
        on_auth(username, password);
        pipeline_.emplace_front(new authpass(password));                
        pipeline_.emplace_front(new authuser(username));
        state_->auth_required = false;
    }
    else
    {
        pipeline_.pop_front();
    }
    current = nullptr;
    if (!pipeline_.empty())
        current = pipeline_.front().get();

    if (current)
    {
        const auto str = current->str();
        if (!str.empty())
            on_send(str + "\r\n");

        if (str.find("AUTHINFO PASS") != std::string::npos)
            LOG_I("AUTHINFO PASS *******");
        else 
            LOG_I(str);

        state_->state = current->state();
    }
    else
    {
        state_->state = state::ready;
    }

    return current == nullptr;
}

bool session::pending() const
{ return !pipeline_.empty(); }

session::error session::get_error() const
{ return state_->error; }

session::state session::get_state() const
{ return state_->state; }

bool session::has_gzip_compress() const 
{ return state_->has_gzip; }

bool session::has_xzver() const 
{ return state_->has_xzver; }

} // newsflash
