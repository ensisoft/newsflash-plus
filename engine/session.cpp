// Copyright (c) 2010-2015 Sami Väisänen, Ensisoft 
//
// http://www.ensisoft.com
// 
// This software is copyrighted software. Unauthorized hacking, cracking, distribution
// and general assing around is prohibited.
// Redistribution and use in source and binary forms, with or without modification,
// without permission are prohibited.
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <newsflash/config.h>
#include <zlib/zlib.h>
#include "session.h"
#include "buffer.h"
#include "nntp.h"
#include "logging.h"
#include "format.h"
#include "types.h"
#include "utility.h"

namespace newsflash
{

struct session::impl {
    bool has_gzip;
    bool has_xzver;
    bool has_modereader;
    bool auth_required;
    bool enable_pipelining;
    bool enable_compression;
    bool have_caps;
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

    virtual bool can_pipeline() const = 0;

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
    virtual bool can_pipeline() const override
    { return false; }

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
        st.have_caps = true;
        return true;
    }

    virtual bool can_pipeline() const override
    { return false; }

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

    virtual bool can_pipeline() const override
    { return false; }

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
    virtual bool can_pipeline() const override
    { return false; }

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
    virtual bool can_pipeline() const override
    { return false; }

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

        const auto code = nntp::scan_response({211, 411, 480}, buff.head(), len);
        if (code == 480)
        {
            st.auth_required = true;
            buff.clear();
            return true;
        }
        else if (code == 411)
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
        st.group = group_;
        return true;
    }

    virtual bool can_pipeline() const override
    { return false; }

    virtual session::state state() const override
    { return session::state::transfer; }

    virtual std::string str() const override
    { return "GROUP " + group_; }

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

        nntp::trailing_comment comment;
        const auto code = nntp::scan_response({222, 423, 420, 430}, buff.head(), len, comment);
        if (code == 423 || code == 420 || code == 430)
        {
            // the output buffer only carries meta information
            // i.e. that there was no content data available.
            if (comment.str.find("dmca") != std::string::npos ||
                comment.str.find("DMCA") != std::string::npos)
                out.set_status(buffer::status::dmca);
            else out.set_status(buffer::status::unavailable);

            out.set_content_type(buffer::type::article);

            // throw away the response line
            buff.split(len);
            return true;
        }

        const auto blen = nntp::find_body(buff.head() + len, buff.size() - len);
        if (blen == 0)
            return false;

        const auto size = len + blen;
        out = buff.split(size);
        out.set_content_type(buffer::type::article);
        out.set_content_length(blen);
        out.set_content_start(len);
        out.set_status(buffer::status::success);

        LOG_I("Read body data ", newsflash::size{blen});
        return true;
    }

    virtual bool can_pipeline() const override
    { return true; }

    virtual session::state state() const override
    { return session::state::transfer; }

    virtual std::string str() const override
    { return "BODY " + messageid_; }
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

    virtual bool can_pipeline() const override
    { return false; }

    virtual session::state state() const override
    { return session::state::quitting; }

    virtual std::string str() const override
    { return "QUIT"; }
private:

};

class session::xover : public session::command
{
public:
    xover(std::string range) : range_(std::move(range))
    {}
    virtual bool parse(buffer& buff, buffer& out, impl& st) override
    {
        const auto len = nntp::find_response(buff.head(), buff.size());
        if (len == 0)
            return false;

        nntp::scan_response({224}, buff.head(), len);

        const auto blen = nntp::find_body(buff.head() + len, buff.size() - len);
        if (blen == 0)
            return false;

        const auto size = len + blen;
        out = buff.split(size);
        out.set_content_type(buffer::type::overview);
        out.set_content_length(blen);
        out.set_content_start(len);
        out.set_status(buffer::status::success);
        
        LOG_I("Read xover data ", newsflash::size{blen});
        return true;
    }

    virtual bool can_pipeline() const override
    { return true; }

    virtual session::state state() const override
    { return session::state::transfer; }

    virtual std::string str() const override
    { return "XOVER " + range_; }
private:
    std::string range_;
};

class session::xovergzip : public session::command
{
public:
    xovergzip(std::string range) : range_(std::move(range)), obytes_(0), ibytes_(0)
    {
        std::memset(&z_, 0, sizeof(z_));
        inflateInit(&z_);
        inflate_done_ = false;
    }
   ~xovergzip()
    {
        inflateEnd(&z_);
    }

    virtual bool parse(buffer& buff, buffer& out, impl& st) override
    {
        if (inflate_done_)
        {
            LOG_D("Rescanning....");
            if (buff.size() >= 3)
            {
                if (!std::strncmp(buff.head(), ".\r\n", 3))
                {
                    LOG_D("Found end of body marker!");
                    buff.pop(3);
                    return true;
                }
            }
        }


        const auto len = nntp::find_response(buff.head(), buff.size());
        if (len == 0)
            return false;

        // 412 no news group selected
        // 420 no article(s) selected
        // 502 no permssion 
        nntp::scan_response({224}, buff.head(), len);        

        if (out.size() == 0)
            out.allocate(MB(1));

        // astraweb supposedly uses a scheme where the zlib deflate'd data is yenc encoded.
        // http://helpdesk.astraweb.com/index.php?_m=news&_a=viewnews&newsid=9
        // but this information is just junk. the data that comes is junk too.
        // the correct way is to call XFEATURE COMPRESS GZIP
        // and then the XOVER data that follows is zlib deflated.

        int err = Z_OK;
        for (;;)
        {
            const auto head = buff.head() + len + ibytes_;
            const auto size = buff.size() - len - ibytes_;
            if (size == 0)
                return false;

            z_.next_in   = (Bytef*)head;
            z_.avail_in  = size;
            z_.next_out  = (Bytef*)out.back();
            z_.avail_out = out.available();
            err = inflate(&z_, Z_NO_FLUSH);

            out.append(z_.total_out - obytes_);
            obytes_ = z_.total_out;
            ibytes_ = z_.total_in;

            if (err < 0)
                break;
            else if (err == Z_BUF_ERROR)
                out.allocate(out.size() * 2);
            else if (err == Z_STREAM_END)
                break;
        }
        inflate_done_ = true;

        if (err != Z_STREAM_END)
        {
            LOG_E("Inflate failed zlib error: ", err);        
            out.set_status(buffer::status::error);       
            return true;
        }
        LOG_I("Inflated header data from ", kb(ibytes_), " to ", kb(obytes_));

        out.set_content_start(0);
        out.set_content_type(buffer::type::overview);
        out.set_status(buffer::status::success);
        out.set_content_length(obytes_);

        const auto head = buff.head() + ibytes_ + len;
        const auto size = buff.size() - ibytes_ - len;
        if (size >= 3) 
        {
            if (!std::strncmp(head, ".\r\n", 3))
            {
                LOG_D("Found end of body marker!");
                buff.pop(ibytes_ + len + 3);
                return true;
            }
        }

        buff.pop(ibytes_ + len);
        // we're still missing the .\r\n that supposedly comes at the end of
        // deflated data. therefore we return false.
        return false;
    }

    virtual bool can_pipeline() const override
    { return true; }

    virtual session::state state() const override
    { return session::state::transfer; }

    virtual std::string str() const override
    { return "XOVER " + range_; }

private:
    std::string range_;
private:
    z_stream z_;
    uLong obytes_;
    uLong ibytes_;
private:
    bool inflate_done_;
};


// the LIST command obtains a valid list of newsgroups on the server
// and information about each group (last, first and count)
class session::list : public session::command
{
public:
    list()
    {}

    virtual bool parse(buffer& buff, buffer& out, impl& st) override
    {
        const auto len = nntp::find_response(buff.head(), buff.size());
        if (len == 0)
            return false;

        // this is only valid response, but check it nevertheless
        // XSUsenet wants to do authentication at LIST command.
        // http://www.xsusenet.com
        const auto code = nntp::scan_response({215, 480}, buff.head(), len);
        if (code == 480)
        {
            st.auth_required = true;
            buff.clear();
            return true;
        }

        const auto blen = nntp::find_body(buff.head() + len, buff.size() - len);
        if (blen == 0)
            return false;

        const auto size = len + blen;
        out = buff.split(size);
        out.set_content_type(buffer::type::grouplist);
        out.set_content_length(blen);
        out.set_content_start(len);
        out.set_status(buffer::status::success);

        LOG_I("Read listing data ", newsflash::size{blen});
        return true;
    }
    virtual bool can_pipeline() const override
    { 
        return false;
    }
    virtual session::state state() const override
    { 
        return session::state::transfer;
    }
    virtual std::string str() const override
    {
        return "LIST";
    }
private:
};

class session::xfeature_compress_gzip : public session::command
{
public:
    virtual bool parse(buffer& buff, buffer& out, impl& st) override
    {
        const auto len = nntp::find_response(buff.head(), buff.size());
        if (len == 0)
            return false;

        // XFEATURE ENABLE COMPRESS is not standard feature
        // specific by the NNTP specs, so im not sure what are the 
        // potential response codes to be used here. hence we just
        // check the initial digit for success/failure.
        // also XSUsenet wants to do a challenge here.

        const auto code = nntp::to_int<int>(buff.head(), 3);
        if (code == 480)
        {
            st.auth_required = true;
            buff.clear();
            return true;
        }

        const auto beg = buff.head();
        if (beg[0] != '2') 
        {
            LOG_W("Compression not supported.");
            st.enable_compression = false;
        }
        buff.clear();
        return true;
    }
    virtual bool can_pipeline() const override
    { return false; }

    virtual session::state state() const override
    { return session::state::init; }

    virtual std::string str() const override
    { return "XFEATURE COMPRESS GZIP"; }

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
    recv_.clear();
    send_.clear();
    state_->auth_required  = false;
    state_->has_gzip       = false;
    state_->has_modereader = false;
    state_->has_xzver      = false;
    state_->have_caps      = false;
    state_->error          = error::none;
    state_->state          = state::none;
    state_->group          = "";
    state_->enable_pipelining = false;
    state_->enable_compression = false;
}

void session::start()
{
    send_.emplace_back(new welcome);
    send_.emplace_back(new getcaps);
    send_.emplace_back(new modereader);  
    if (state_->enable_compression)
        send_.emplace_back(new xfeature_compress_gzip);
}

void session::quit()
{
    send_.emplace_back(new class quit);
}

void session::change_group(std::string name)
{
    if (name == state_->group)
        return;
    send_.emplace_back(new group(std::move(name)));
}

void session::retrieve_group_info(std::string name)
{
    send_.emplace_back(new group(std::move(name)));
}

void session::retrieve_article(std::string messageid)
{
    send_.emplace_back(new body(std::move(messageid)));
}

void session::retrieve_headers(std::string range)
{
    if (state_->enable_compression)
    {
        send_.emplace_back(new xovergzip(std::move(range)));
    }
    else
    {
        send_.emplace_back(new xover(std::move(range)));
    }
}

void session::retrieve_list()
{
    send_.emplace_back(new list);
}

void session::ping()
{
    // NNTP doesn't have a "real" ping built into it
    // so we simply send a mundane command (change group)
    // to perform some activity on the transmission line.
    send_.emplace_back(new group("keeping.session.alive"));
}

bool session::send_next()
{
    if (send_.empty())
        return false;

    // command pipelining is a mechanism which allows us to send
    // multiple commands back-to-back and then receive multiple responses
    // back to back. 
    // so instead of sending BODY 1 and then receiving the article data for body1
    // and then sending BODY 2 and receiving data we can do
    // BODY 1\r\n BODY 2\r\n ... BODY N\r\n
    // and then read body1, body2 ... bodyN article datas

    // however in case of commands that may not be pipelined
    // we may not send any commands before the response for the 
    // non-pipelined command has been received.

    if (!recv_.empty())
    {
        // if the last command we have sent is non-pipelineable
        // we must wait for the response untill sending more commands.
        const auto& last = recv_.back();
        if (!last->can_pipeline())
            return false;

        if (!state_->enable_pipelining)
            return false;
    }

    for (;;)
    {
        auto& next = send_.front();

        const auto& str = next->str();
        if (!str.empty())
        {
            if (str.find("AUTHINFO PASS") != std::string::npos)
                LOG_I("AUTHINFO PASS ****");
            else LOG_I(str);

            on_send(str + "\r\n");
        }

        state_->state = next->state();

        recv_.push_back(std::move(next));
        send_.pop_front();

        // if pipelining is not enable we only run this loop once,
        // essentially reducing the operation to send/recv pairs
        if (!state_->enable_pipelining)
            break;

        if (send_.empty())
            break;

        // first non-pipelineable command will stall the sending queue
        if (!send_.front()->can_pipeline())
            break;
    }
    return true;
}


bool session::recv_next(buffer& buff, buffer& out)
{
    assert(!recv_.empty());

    auto& next = recv_.front();

    std::string response;
    const auto len = nntp::find_response(buff.head(), buff.size());
    if (len != 0)
        response.append(buff.head(), len-2);

    if (!next->parse(buff, out, *state_))
        return false;

    if (response[0] == '5')
        LOG_E(response);
    else if (response[0] == '4')
        LOG_W(response);
    else LOG_I(response);

    if (state_->error != error::none)
    {
        state_->state = state::error;
        recv_.clear();
        send_.clear();
        return true;
    }
    else if (state_->auth_required)
    {
        std::string username;
        std::string password;
        on_auth(username, password);
        if (recv_.size() != 1)
            throw std::runtime_error("authentication requested during pipelined commands");

        send_.push_front(std::move(next));
        if (!state_->have_caps)
            send_.emplace_front(new getcaps);

        send_.emplace_front(new authpass(password));                
        send_.emplace_front(new authuser(username));

        recv_.pop_front();
        state_->auth_required = false;
    }
    else
    {
        recv_.pop_front();
    }
    if (recv_.empty())
        state_->state = state::ready;

    return true;
}

void session::clear()
{
    recv_.clear();
    send_.clear();
}

bool session::pending() const
{ return !send_.empty() || !recv_.empty(); }

void session::enable_pipelining(bool on_off)
{ state_->enable_pipelining = on_off; }

void session::enable_compression(bool on_off)
{ state_->enable_compression = on_off; }

session::error session::get_error() const
{ return state_->error; }

session::state session::get_state() const
{ return state_->state; }

bool session::has_gzip_compress() const 
{ return state_->has_gzip; }

bool session::has_xzver() const 
{ return state_->has_xzver; }

} // newsflash




