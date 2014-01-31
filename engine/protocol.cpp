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

//#include <newsflash/config.h>

#include <boost/algorithm/string/case_conv.hpp>
#include <string>
#include "protocmd.h"
#include "protocol.h"
#include "buffer.h"

namespace newsflash
{

protocol::protocol() 
    : has_compress_gzip_(false), has_xzver_(false), has_mode_reader_(false), is_compressed_(false)
{}

protocol::~protocol()
{}

void protocol::connect()
{
    has_compress_gzip_ = false;
    has_xzver_         = false;
    has_mode_reader_   = false;    
    is_compressed_     = false;
    group_             = "";

    auto ret = transact(nntp::cmd_welcome{});

    if (ret == nntp::cmd_welcome::SERVICE_TEMPORARILY_UNAVAILABLE)
        throw exception("service temporarily unavailable", exception::code::service_temporarily_unavailable);
    else if (ret == nntp::cmd_welcome::SERVICE_PERMANENTLY_UNAVAILABLE)
        throw exception("service permanently unavailable", exception::code::service_permanently_unavailable);
    
    // common nntp extensions (rfc980) doesn't actually seem to specify
    // when the request to authenticate can happen. Presumably it could happen
    // at any command, but in practice it seems that the session is requested 
    // to be authenticated only at the start.

    nntp::cmd_capabilities cmd;
    transact(&cmd);

    has_compress_gzip_ = cmd.has_compress_gzip;
    has_xzver_         = cmd.has_xzver;
    has_mode_reader_   = cmd.has_mode_reader;

    transact(nntp::cmd_mode_reader{});
}


void protocol::ping()
{
    // there's no actual "ping" in NNTP specification
    // so we fabricate this by sending a mundane command.
    transact(nntp::cmd_group {"send.ping.to.keep.connection.alive"});
}

void protocol::quit()
{
    transact(nntp::cmd_quit{});
}

bool protocol::change_group(const std::string& groupname)
{
    if (group_ == groupname)
        return true;

    const auto code = transact(nntp::cmd_group{groupname});
    if (code != nntp::cmd_group::SUCCESS)
        return false;

    group_ = groupname;
    return true;
}

bool protocol::query_group(const std::string& groupname, groupinfo& info)
{
    nntp::cmd_group cmd { groupname };

    const auto code = transact(&cmd);
    if (code != nntp::cmd_group::SUCCESS)
        return false;

    group_ = groupname;
    
    info.high_water_mark = cmd.high;
    info.low_water_mark = cmd.low;
    info.article_count = cmd.count;
    return true;
}

protocol::status protocol::download_article(const std::string& article, buffer& buff)
{ 
    typedef nntp::cmd_body<buffer> command_t;

    command_t cmd {buff, article};

    const auto code = transact(&cmd);
    if (code == command_t::SUCCESS)
    {
        buff.configure(cmd.size, cmd.offset);
        return status::success;
    }

    const std::string& reason = boost::algorithm::to_lower_copy(cmd.reason);
    if (reason.find("dmca") != std::string::npos)
        return status::dmca;

    return status::unavailable;
}

bool protocol::download_overview(const std::string& first, const std::string& last, buffer& buff)
{
    if (has_xzver_ && !is_compressed_)
    {
        // try to turn on the compression
        const auto code = transact(nntp::cmd_enable_compress_gzip{});
        if (code == nntp::cmd_enable_compress_gzip::SUCCESS)
            is_compressed_ = true;

    }

    typedef nntp::cmd_xzver<buffer> xzver_t;        
    typedef nntp::cmd_xover<buffer> xover_t;


    if (is_compressed_)
    {
        xzver_t cmd {buff, first, last };

        if (transact(&cmd) == xzver_t::SUCCESS)
        {
            buff.configure(cmd.size, cmd.offset);
            return true;
        }
    }
    else
    {
        xover_t cmd {buff, first, last};

        if (transact(&cmd) == xover_t::SUCCESS)
        {
            buff.configure(cmd.size, cmd.offset);
            return true;
        }
    }
    return false;
}

bool protocol::download_list(buffer& buff)
{
    typedef nntp::cmd_list<buffer> command_t;

    command_t cmd {buff};

    if (transact(&cmd) == command_t::SUCCESS)
    {
        buff.configure(cmd.size, cmd.offset);
        return true;
    }
    return false;
}

void protocol::authenticate()
{
    std::string username;
    std::string password;
    if (on_auth)
        on_auth(username, password);

    nntp::cmd_auth_user user {username};
    user.cmd_recv = on_recv;
    user.cmd_send = on_send;
    user.cmd_log  = on_log;

    auto ret = user.transact();
    if (ret == nntp::PASSWORD_REQUIRED)
    {
        nntp::cmd_auth_pass pass {password};
        pass.cmd_recv = on_recv;
        pass.cmd_send = on_send;
        pass.cmd_log  = on_log;

        ret = pass.transact();
    }
    if (ret != nntp::AUTHENTICATION_ACCEPTED)
        throw exception("authentication failed", exception::code::authentication_failed);    
}

} // newsflash
