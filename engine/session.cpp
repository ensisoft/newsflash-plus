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

#include "assert.h"
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
    bool enable_pipelining = false;
    bool enable_compression = false;
    bool enable_skipping = true;
    bool have_caps = false;
    std::string username;
    std::string password;
    std::string group;
    Session::Error error = Session::Error::None;
    Session::State state = Session::State::None;
    Session::SendCallback on_send;
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

    virtual bool is_compressed() const
    { return false; }

    // get the state represented by this command
    virtual Session::State state() const = 0;

    // get the error if any
    virtual Session::Error error() const = 0;

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

    virtual Session::Error error() const override
    { return Session::Error::None; }

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

    virtual Session::Error error() const override
    { return Session::Error::None; }

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

        // 281 authentication accepted
        // 381 more authentication required
        // 480 authentication required  // what??
        // 482 authentication rejected
        // 502 no permission
        const auto code = nntp::scan_response({281, 381, 482, 502}, buff.Head(), len);
        if (code == 482)
            error_ = Error::AuthenticationRejected;
        else if (code == 502)
            error_ = Error::NoPermission;

        buff.Clear();
        return true;
    }

    virtual bool can_pipeline() const override
    { return false; }

    virtual Session::State state() const override
    { return Session::State::Authenticate; }

    virtual Session::Error error() const override
    { return error_; }

    virtual std::string str() const override
    { return "AUTHINFO USER " + username_; }
private:
    std::string username_;
    Session::Error error_ = Session::Error::None;
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
            error_ = Error::AuthenticationRejected;
        else if (code == 502)
            error_ = Error::NoPermission;

        buff.Clear();
        return true;
    }
    virtual bool can_pipeline() const override
    { return false; }

    virtual Session::State state() const override
    { return Session::State::Authenticate; }

    virtual Session::Error error() const override
    { return error_; }

    virtual std::string str() const override
    { return "AUTHINFO PASS " + password_; }
private:
    std::string password_;
    Session::Error error_ = Session::Error::None;
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

        nntp::scan_response({200, 201}, buff.Head(), len);

        buff.Clear();
        return true;
    }
    virtual bool can_pipeline() const override
    { return false; }

    virtual Session::State state() const override
    { return Session::State::Init; }

    virtual Session::Error error() const override
    { return Session::Error::None; }

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

        const auto code = nntp::scan_response({211, 411}, buff.Head(), len);
        if (code == 411)
        {
            out.SetStatus(Buffer::Status::Unavailable);
        }
        else
        {
            out = buff.Split(len);
            out.SetContentLength(len);
            out.SetContentStart(0);
            out.SetStatus(Buffer::Status::Success);
            st.group = group_;
        }
        out.SetContentType(Buffer::Type::GroupInfo);
        return true;
    }

    virtual bool can_pipeline() const override
    { return false; }

    virtual Session::State state() const override
    { return Session::State::Transfer; }

    virtual Session::Error error() const override
    { return Session::Error::None; }

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

        //
        // 3.1.3.
        // Responses
        // 220 n <a> article retrieved - head and body follow (n = article number, <a> = message-id)
        // 221 n <a> article retrieved - head follows
        // 222 n <a> article retrieved - body follows
        // 223 n <a> article retrieved - request text separately
        // 412 no newsgroup has been selected
        // 420 no current article has been selected
        // 423 no such article number in this group
        // 430 no such article found
        //
        // giganews seems to respond also with 451 cancelled.
        // (whatever the fuck that means...)

        nntp::trailing_comment comment;
        const auto code = nntp::scan_response({222, 423, 420, 430, 451}, buff.Head(), len, comment);
        if (code == 423 || code == 420 || code == 430 || code == 451)
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

    virtual Session::Error error() const override
    { return Session::Error::None; }

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

    virtual Session::Error error() const override
    { return Session::Error::None; }

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

    virtual Session::Error error() const override
    { return Session::Error::None; }

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

        if (!out.GetCapacity())
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

    virtual bool is_compressed() const override
    { return true; }

    virtual Session::State state() const override
    { return Session::State::Transfer; }

    virtual Session::Error error() const override
    { return Session::Error::None; }

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
        const auto code = nntp::scan_response({215}, buff.Head(), len);
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
    { return false; }

    virtual Session::State state() const override
    { return Session::State::Transfer; }

    virtual Session::Error error() const override
    { return Session::Error::None; }

    virtual std::string str() const override
    { return "LIST"; }

private:
};

class Session::listgzip : public Session::command
{
public:
    listgzip()
    {
        std::memset(&z_, 0, sizeof(z_));
        inflateInit(&z_);
    }
   ~listgzip()
    {
        inflateEnd(&z_);
    }
    virtual bool parse(Buffer& buff, Buffer& out, impl& st) override
    {
        if (inflate_done_)
        {
            LOG_D("Rescanning ....");
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

        nntp::scan_response({215}, buff.Head(), len);

        if (!out.GetCapacity())
            out.Allocate(MB(5));

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
        LOG_I("Inflated list data from ", kb(ibytes_), " to ", kb(obytes_));

        out.SetContentStart(0);
        out.SetContentType(Buffer::Type::GroupList);
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
        // we're still missing the ".\r\n" that supposedly comes at the
        // end of the deflated data. therefore we return false
        return false;
    }

    virtual bool can_pipeline() const override
    { return false; }

    virtual bool is_compressed() const override
    { return true; }

    virtual Session::State state() const override
    { return Session::State::Transfer; }

    virtual Session::Error error() const override
    { return Session::Error::None; }

    virtual std::string str() const override
    { return "LIST"; }
private:
    z_stream z_;
    uLong obytes_ = 0;
    uLong ibytes_ = 0;
private:
    bool inflate_done_ = false;
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

    virtual Session::Error error() const override
    { return Session::Error::None; }

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
    state_->has_gzip       = false;
    state_->has_modereader = false;
    state_->has_xzver      = false;
    state_->have_caps      = false;
    state_->error          = Error::None;
    state_->state          = State::None;
    state_->group          = "";
    state_->enable_pipelining = false;
    state_->enable_compression = false;
    state_->enable_skipping = true;
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
    if ((name == state_->group) && state_->enable_skipping)
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
    if (state_->enable_compression)
    {
        send_.emplace_back(new listgzip);
    }
    else
    {
        send_.emplace_back(new list);
    }
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

            state_->on_send(str + "\r\n");
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
    ASSERT(!recv_.empty());

    auto& next = recv_.front();

    const auto len = nntp::find_response(buff.Head(), buff.GetSize());
    const auto ptr = buff.Head();
    if (len == 0)
        return false;
    const std::string response(ptr, len-2);

    // RFC 977/3977 specifies a list of general responses that can happen
    // for any command.
    // the complete list has responses in 1xx, 2xx, 4xx and 5xx category.
    // we can handle them in general manner here instead of having
    // each command check for those.
    // however we're only interested in a few possible responses.
    // the rest will either be handled case-by-case basis in the command
    // implementations (if the command knows how to deal with it) or
    // will trigger a nntp::exception which will become a protocol error.
    //
    // 400 service discontinued.
    // - I haven't seen this in practice so let's not handle this for now.
    //
    // 502 access restriction
    // - used to indicate out of quota / permission denied
    //
    const auto code = nntp::to_int<int>(buff.Head(), 3);
    if (code == 480)
    {
        LOG_I(response);

        if (recv_.size() != 1)
        {
            LOG_E("Authentication requested during pipelined commands");
            state_->state = State::Error;
            state_->error = Error::Protocol;
            recv_.clear();
            send_.clear();
            return true;
        }

        const bool is_get_caps_command = (dynamic_cast<getcaps*>(next.get()) != nullptr);

        // push for repeat
        send_.push_front(std::move(next));

        if (!state_->have_caps && !is_get_caps_command)
        {
            // some servers don't respond to CAPABILITIES properly if we're not
            // authenticated but just give some 4xx error.
            // so if authentication is requested but we don't yet have caps
            // then we'll try to repeat the CAPABILITIES command. (except if authentication
            // was requested when doing CAPABILITIES in the first place).
            send_.emplace_front(new getcaps);
        }
        // todo: only send password when 381 is received
        send_.emplace_front(new authpass(state_->password));
        send_.emplace_front(new authuser(state_->username));
        recv_.pop_front();
        buff.Clear();
        return true;
    }
    else if (code == 502)
    {
        // access restriction or permission denied
        LOG_E(response);
        state_->state = State::Error;
        state_->error = Error::NoPermission;
        recv_.clear();
        send_.clear();
        return true;
    }

    try
    {
        if (!next->parse(buff, out, *state_))
            return false;

        const auto err = next->error();
        if (err != Session::Error::None)
        {
            state_->state = State::Error;
            state_->error = err;
            recv_.clear();
            send_.clear();
            return true;
        }
    }
    catch (const nntp::exception& e)
    {
        LOG_E(response);
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

    recv_.pop_front();

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

bool Session::IsCurrentCommandCompressed() const
{
    ASSERT(!recv_.empty());

    const auto& next = recv_.front();
    return next->is_compressed();
}

bool Session::IsCurrentCommandContent() const
{
    ASSERT(!recv_.empty());

    const auto& next = recv_.front();
    return next->state() == State::Transfer;
}

void Session::SetEnablePipelining(bool on_off)
{ state_->enable_pipelining = on_off; }

void Session::SetEnableCompression(bool on_off)
{ state_->enable_compression = on_off; }

void Session::SetEnableSkipRedundant(bool on_off)
{ state_->enable_skipping = on_off; }

void Session::SetCredentials(const std::string& username, const std::string& password)
{
    state_->username = username;
    state_->password = password;
}

void Session::SetSendCallback(const SendCallback& callback)
{
    state_->on_send = callback;
}

Session::Error Session::GetError() const
{ return state_->error; }

Session::State Session::GetState() const
{ return state_->state; }

bool Session::HasGzipCompress() const
{ return state_->has_gzip; }

bool Session::HasXzver() const
{ return state_->has_xzver; }

} // newsflash




