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

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <zlib/zlib.h>
#include "newsflash/warnpop.h"

#include "session.h"
#include "buffer.h"
#include "nntp.h"
#include "logging.h"
#include "format.h"
#include "types.h"
#include "utility.h"

namespace newsflash
{

struct Session::impl {
    bool has_gzip = false;
    bool has_xzver = false;
    bool has_modereader = false;
    bool auth_required = false;
    bool enable_pipelining = false;
    bool enable_compression = false;
    bool have_caps = false;
    std::string user;
    std::string pass;
    std::string group;
    Session::Error error = Session::Error::None;
    Session::State state = Session::State::None;
};

// command encapsulates a request/response transaction.
class Session::command
{
public:
    virtual ~command() = default;

    // parse the input Buffer and store the possible data contents
    // into output Buffer.
    virtual bool parse(Buffer& buff, Buffer& out, impl& st) = 0;

    virtual bool can_pipeline() const = 0;

    // get the state represented by this command
    virtual Session::State state() const = 0;

    // get the nntp command string
    virtual std::string str() const = 0;
protected:
private:
};


// read initial greeting from the server
class Session::welcome : public Session::command
{
public:
    virtual bool parse(Buffer& buff, Buffer& out, Session::impl& st) override
    {
        const auto len = nntp::find_response(buff.Head(), buff.GetSize());
        if (len == 0)
            return false;

        // i have never seen these actually being used so we're just
        // going to pretend they dont exist.
        // 400 service temporarily unavailable
        // 502 service permantly unavailable
        nntp::scan_response({200, 201}, buff.Head(), len);
        buff.Clear();
        return true;
    }
    virtual bool can_pipeline() const override
    { return false; }

    virtual Session::State state() const override
    { return Session::State::Init; }

    virtual std::string str() const override
    { return ""; }
private:
};

// query server for supported capabilities
class Session::getcaps : public Session::command
{
public:
    virtual bool parse(Buffer& buff, Buffer& out, Session::impl& st) override
    {
        const auto res = nntp::find_response(buff.Head(), buff.GetSize());
        if (res == 0)
            return false;

        // usenet.premiumize.me responds to CAPABILITIES with 400 instead of 500.
        // https://github.com/ensisoft/newsflash-plus/issues/29
        const auto code = nntp::to_int<int>(buff.Head(), 3);
        //const auto code = nntp::scan_response({101, 480, 500}, buff.Head(), res);
        if (code != 101)
        {
            if (code == 480)
                st.auth_required = true;
            buff.Clear();
            return true;
        }

        const auto ptr = buff.Head() + res;
        const auto len = nntp::find_body(ptr, buff.GetSize() - res);
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
        buff.Clear();
        st.have_caps = true;
        return true;
    }

    virtual bool can_pipeline() const override
    { return false; }

    virtual Session::State state() const override
    { return Session::State::Init; }

    virtual std::string str() const override
    { return "CAPABILITIES"; }
private:
};

// perform user authentication (username)
class Session::authuser : public Session::command
{
public:
    authuser(const std::string& username) : username_(username)
    {}

    virtual bool parse(Buffer& buff, Buffer& out, impl& st) override
    {
        const auto len = nntp::find_response(buff.Head(), buff.GetSize());
        if (len == 0)
            return false;

        // we also have 482 and 502 responses in the spec but
        // consider these to be a "programmer error"
        // 482 authentication command issued out of sequence
        // 502 command unavailable

        const auto code = nntp::scan_response({281, 381, 482, 502}, buff.Head(), len);
        if (code == 482)
            st.error = Error::AuthenticationRejected;
        else if (code == 502)
            st.error = Error::NoPermission;

        buff.Clear();
        return true;
    }

    virtual bool can_pipeline() const override
    { return false; }

    virtual Session::State state() const override
    { return Session::State::Authenticate; }

    virtual std::string str() const override
    { return "AUTHINFO USER " + username_; }
private:
    std::string username_;
};

// perform user authenticaton (password)
class Session::authpass : public Session::command
{
public:
    authpass(const std::string& password) : password_(password)
    {}

    virtual bool parse(Buffer& buff, Buffer& out, impl& st) override
    {
        const auto len = nntp::find_response(buff.Head(), buff.GetSize());
        if ( len== 0)
            return false;

        const auto code = nntp::scan_response({281, 482, 502}, buff.Head(), len);
        if (code == 482)
            st.error = Error::AuthenticationRejected;
        else if (code == 502)
            st.error = Error::NoPermission;

        buff.Clear();
        return true;
    }
    virtual bool can_pipeline() const override
    { return false; }

    virtual Session::State state() const override
    { return Session::State::Authenticate; }

    virtual std::string str() const override
    { return "AUTHINFO PASS " + password_; }
private:
    std::string password_;
};

// set mode reader for server
class Session::modereader : public Session::command
{
public:
    virtual bool parse(Buffer& buff, Buffer& out, impl& st) override
    {
        const auto len = nntp::find_response(buff.Head(), buff.GetSize());
        if (len == 0)
            return false;
        const auto code = nntp::scan_response({200, 201, 480}, buff.Head(), len);
        if (code == 480)
            st.auth_required = true;

        buff.Clear();
        return true;
    }
    virtual bool can_pipeline() const override
    { return false; }

    virtual Session::State state() const override
    { return Session::State::Init; }

    virtual std::string str() const override
    { return "MODE READER"; }
private:
};

// change group/store group information into data Buffer
class Session::group : public Session::command
{
public:
    group(const std::string& name) : group_(name)
    {}

    virtual bool parse(Buffer& buff, Buffer& out, impl& st) override
    {
        const auto len = nntp::find_response(buff.Head(), buff.GetSize());
        if (len == 0)
            return false;

        const auto code = nntp::scan_response({211, 411, 480}, buff.Head(), len);
        if (code == 480)
        {
            st.auth_required = true;
            buff.Clear();
            return true;
        }
        else if (code == 411)
        {
            out.SetStatus(Buffer::Status::Unavailable);
        }
        else
        {
            out = buff.Split(len);
            out.SetContentLength(len);
            out.SetContentStart(0);
            out.SetStatus(Buffer::Status::Success);
        }
        out.SetContentType(Buffer::Type::GroupInfo);
        st.group = group_;
        return true;
    }

    virtual bool can_pipeline() const override
    { return false; }

    virtual Session::State state() const override
    { return Session::State::Transfer; }

    virtual std::string str() const override
    { return "GROUP " + group_; }

private:
    std::string group_;
private:
    std::size_t count_ = 0;
    std::size_t low_   = 0;
    std::size_t high_  = 0;
};

// request article data from the server. store into output Buffer
class Session::body : public Session::command
{
public:
    body(const std::string& messageid) : messageid_(messageid)
    {}

    virtual bool parse(Buffer& buff, Buffer& out, impl& st) override
    {
        const auto len = nntp::find_response(buff.Head(), buff.GetSize());
        if (len == 0)
            return false;

        nntp::trailing_comment comment;
        const auto code = nntp::scan_response({222, 423, 420, 430}, buff.Head(), len, comment);
        if (code == 423 || code == 420 || code == 430)
        {
            // the output Buffer only carries meta information
            // i.e. that there was no content data available.
            if (comment.str.find("dmca") != std::string::npos ||
                comment.str.find("DMCA") != std::string::npos)
                out.SetStatus(Buffer::Status::Dmca);
            else out.SetStatus(Buffer::Status::Unavailable);

            out.SetContentType(Buffer::Type::Article);

            // throw away the response line
            buff.Split(len);
            return true;
        }

        const auto blen = nntp::find_body(buff.Head() + len, buff.GetSize() - len);
        if (blen == 0)
            return false;

        const auto GetSize = len + blen;
        out = buff.Split(GetSize);
        out.SetContentType(Buffer::Type::Article);
        out.SetContentLength(blen);
        out.SetContentStart(len);
        out.SetStatus(Buffer::Status::Success);

        LOG_I("Read body data ", newsflash::size{blen});
        return true;
    }

    virtual bool can_pipeline() const override
    { return true; }

    virtual Session::State state() const override
    { return Session::State::Transfer; }

    virtual std::string str() const override
    { return "BODY " + messageid_; }
private:
    std::string messageid_;
};


// quit Session.
class Session::quit : public Session::command
{
public:

    virtual bool parse(Buffer& buff, Buffer& out, impl& st) override
    {
        const auto len = nntp::find_response(buff.Head(), buff.GetSize());
        if (len == 0)
            return false;

        nntp::scan_response({205}, buff.Head(), len);
        return true;
    }

    virtual bool can_pipeline() const override
    { return false; }

    virtual Session::State state() const override
    { return Session::State::Quitting; }

    virtual std::string str() const override
    { return "QUIT"; }
private:

};

class Session::xover : public Session::command
{
public:
    xover(const std::string& range) : range_(range)
    {}
    virtual bool parse(Buffer& buff, Buffer& out, impl& st) override
    {
        const auto len = nntp::find_response(buff.Head(), buff.GetSize());
        if (len == 0)
            return false;

        nntp::scan_response({224}, buff.Head(), len);

        const auto blen = nntp::find_body(buff.Head() + len, buff.GetSize() - len);
        if (blen == 0)
            return false;

        const auto size = len + blen;
        out = buff.Split(size);
        out.SetContentType(Buffer::Type::Overview);
        out.SetContentLength(blen);
        out.SetContentStart(len);
        out.SetStatus(Buffer::Status::Success);

        LOG_I("Read xover data ", newsflash::size{blen});
        return true;
    }

    virtual bool can_pipeline() const override
    { return true; }

    virtual Session::State state() const override
    { return Session::State::Transfer; }

    virtual std::string str() const override
    { return "XOVER " + range_; }
private:
    std::string range_;
};

class Session::xovergzip : public Session::command
{
public:
    xovergzip(const std::string& range) : range_(range)
    {
        std::memset(&z_, 0, sizeof(z_));
        inflateInit(&z_);
    }
   ~xovergzip()
    {
        inflateEnd(&z_);
    }

    virtual bool parse(Buffer& buff, Buffer& out, impl& st) override
    {
        if (inflate_done_)
        {
            LOG_D("Rescanning....");
            if (buff.GetSize() >= 3)
            {
                if (!std::strncmp(buff.Head(), ".\r\n", 3))
                {
                    LOG_D("Found end of body marker!");
                    buff.Pop(3);
                    return true;
                }
            }
        }


        const auto len = nntp::find_response(buff.Head(), buff.GetSize());
        if (len == 0)
            return false;

        // 412 no news group selected
        // 420 no article(s) selected
        // 502 no permssion
        nntp::scan_response({224}, buff.Head(), len);

        if (out.GetCapacity())
            out.Allocate(MB(1));

        // astraweb supposedly uses a scheme where the zlib deflate'd data is yenc encoded.
        // http://helpdesk.astraweb.com/index.php?_m=news&_a=viewnews&newsid=9
        // but this information is just junk. the data that comes is junk too.
        // the correct way is to call XFEATURE COMPRESS GZIP
        // and then the XOVER data that follows is zlib deflated.

        int err = Z_OK;
        for (;;)
        {
            const auto head = buff.Head() + len + ibytes_;
            const auto size = buff.GetSize() - len - ibytes_;
            if (size == 0)
                return false;

            z_.next_in   = (Bytef*)head;
            z_.avail_in  = size;
            z_.next_out  = (Bytef*)out.Back();
            z_.avail_out = out.GetAvailableBytes();
            err = inflate(&z_, Z_NO_FLUSH);

            out.Append(z_.total_out - obytes_);
            obytes_ = z_.total_out;
            ibytes_ = z_.total_in;

            if (err < 0)
                break;
            else if (err == Z_BUF_ERROR)
                out.Allocate(out.GetSize() * 2);
            else if (err == Z_STREAM_END)
                break;
        }
        inflate_done_ = true;

        if (err != Z_STREAM_END)
        {
            LOG_E("Inflate failed zlib error: ", err);
            out.SetStatus(Buffer::Status::Error);
            return true;
        }
        LOG_I("Inflated header data from ", kb(ibytes_), " to ", kb(obytes_));

        out.SetContentStart(0);
        out.SetContentType(Buffer::Type::Overview);
        out.SetStatus(Buffer::Status::Success);
        out.SetContentLength(obytes_);

        const auto head = buff.Head() + ibytes_ + len;
        const auto size = buff.GetSize() - ibytes_ - len;
        if (size >= 3)
        {
            if (!std::strncmp(head, ".\r\n", 3))
            {
                LOG_D("Found end of body marker!");
                buff.Pop(ibytes_ + len + 3);
                return true;
            }
        }

        buff.Pop(ibytes_ + len);
        // we're still missing the .\r\n that supposedly comes at the end of
        // deflated data. therefore we return false.
        return false;
    }

    virtual bool can_pipeline() const override
    { return true; }

    virtual Session::State state() const override
    { return Session::State::Transfer; }

    virtual std::string str() const override
    { return "XOVER " + range_; }

private:
    std::string range_;
private:
    z_stream z_;
    uLong obytes_ = 0;
    uLong ibytes_ = 0;
private:
    bool inflate_done_ = false;
};


// the LIST command obtains a valid list of newsgroups on the server
// and information about each group (last, first and count)
class Session::list : public Session::command
{
public:
    virtual bool parse(Buffer& buff, Buffer& out, impl& st) override
    {
        const auto len = nntp::find_response(buff.Head(), buff.GetSize());
        if (len == 0)
            return false;

        // this is only valid response, but check it nevertheless
        // XSUsenet wants to do authentication at LIST command.
        // http://www.xsusenet.com
        const auto code = nntp::scan_response({215, 480}, buff.Head(), len);
        if (code == 480)
        {
            st.auth_required = true;
            buff.Clear();
            return true;
        }

        const auto blen = nntp::find_body(buff.Head() + len, buff.GetSize() - len);
        if (blen == 0)
            return false;

        const auto size = len + blen;
        out = buff.Split(size);
        out.SetContentType(Buffer::Type::GroupList);
        out.SetContentLength(blen);
        out.SetContentStart(len);
        out.SetStatus(Buffer::Status::Success);

        LOG_I("Read listing data ", newsflash::size{blen});
        return true;
    }
    virtual bool can_pipeline() const override
    {
        return false;
    }
    virtual Session::State state() const override
    {
        return Session::State::Transfer;
    }
    virtual std::string str() const override
    {
        return "LIST";
    }
private:
};

class Session::xfeature_compress_gzip : public Session::command
{
public:
    virtual bool parse(Buffer& buff, Buffer& out, impl& st) override
    {
        const auto len = nntp::find_response(buff.Head(), buff.GetSize());
        if (len == 0)
            return false;

        // XFEATURE ENABLE COMPRESS is not standard feature
        // specific by the NNTP specs, so im not sure what are the
        // potential response codes to be used here. hence we just
        // check the initial digit for success/failure.
        // also XSUsenet wants to do a challenge here.

        const auto code = nntp::to_int<int>(buff.Head(), 3);
        if (code == 480)
        {
            st.auth_required = true;
            buff.Clear();
            return true;
        }

        const auto beg = buff.Head();
        if (beg[0] != '2')
        {
            LOG_W("Compression not supported.");
            st.enable_compression = false;
        }
        buff.Clear();
        return true;
    }
    virtual bool can_pipeline() const override
    { return false; }

    virtual Session::State state() const override
    { return Session::State::Init; }

    virtual std::string str() const override
    { return "XFEATURE COMPRESS GZIP"; }

private:

};

Session::Session() : state_(new impl)
{
    Reset();
}

Session::~Session()
{}

void Session::Reset()
{
    recv_.clear();
    send_.clear();
    state_->auth_required  = false;
    state_->has_gzip       = false;
    state_->has_modereader = false;
    state_->has_xzver      = false;
    state_->have_caps      = false;
    state_->error          = Error::None;
    state_->state          = State::None;
    state_->group          = "";
    state_->enable_pipelining = false;
    state_->enable_compression = false;
}

void Session::Start(bool authenticate_immediately)
{
    send_.emplace_back(new welcome);
    send_.emplace_back(new getcaps);
    send_.emplace_back(new modereader);
    if (state_->enable_compression)
        send_.emplace_back(new xfeature_compress_gzip);

    // it's useful for account testing purposes to try perform
    // authentication immediately to see if the user credentials
    // are working or not.
    // ufortunatately there's no *real* way of doing it so instead
    // we'll try to just change the group to something that should
    // exist on the server.
    if (authenticate_immediately)
        send_.emplace_back(new group("alt.binaries.test"));
}

void Session::Quit()
{
    send_.emplace_back(new class quit);
}

void Session::ChangeGroup(const std::string& name)
{
    if (name == state_->group)
        return;
    send_.emplace_back(new group(name));
}

void Session::RetrieveGroupInfo(const std::string& name)
{
    send_.emplace_back(new group(name));
}

void Session::RetrieveArticle(const std::string& messageid)
{
    send_.emplace_back(new body(messageid));
}

void Session::RetrieveHeaders(const std::string& range)
{
    if (state_->enable_compression)
    {
        send_.emplace_back(new xovergzip(range));
    }
    else
    {
        send_.emplace_back(new xover(range));
    }
}

void Session::RetrieveList()
{
    send_.emplace_back(new list);
}

void Session::Ping()
{
    // NNTP doesn't have a "real" ping built into it
    // so we simply send a mundane command (change group)
    // to perform some activity on the transmission line.
    send_.emplace_back(new group("keeping.Session.alive"));
}

bool Session::SendNext()
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


bool Session::RecvNext(Buffer& buff, Buffer& out)
{
    assert(!recv_.empty());

    auto& next = recv_.front();

    std::string response;
    const auto len = nntp::find_response(buff.Head(), buff.GetSize());
    if (len != 0)
        response.append(buff.Head(), len-2);

    try
    {
        if (!next->parse(buff, out, *state_))
            return false;
    }
    catch (const nntp::exception& e)
    {
        state_->state = State::Error;
        state_->error = Error::Protocol;
        recv_.clear();
        send_.clear();
        return true;
    }

    if (response[0] == '5')
        LOG_E(response);
    else if (response[0] == '4')
        LOG_W(response);
    else LOG_I(response);

    if (state_->error != Error::None)
    {
        state_->state = State::Error;
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
        state_->state = State::Ready;

    return true;
}

void Session::Clear()
{
    recv_.clear();
    send_.clear();
}

bool Session::HasPending() const
{ return !send_.empty() || !recv_.empty(); }

void Session::SetEnablePipelining(bool on_off)
{ state_->enable_pipelining = on_off; }

void Session::SetEnableCompression(bool on_off)
{ state_->enable_compression = on_off; }

Session::Error Session::GetError() const
{ return state_->error; }

Session::State Session::GetState() const
{ return state_->state; }

bool Session::HasGzipCompress() const
{ return state_->has_gzip; }

bool Session::HasXzver() const
{ return state_->has_xzver; }

} // newsflash




