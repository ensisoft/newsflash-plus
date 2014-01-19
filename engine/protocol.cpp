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

#include <string>
#include "protocmd.h"
#include "protocol.h"
#include "buffer.h"

namespace newsflash
{

protocol::protocol() 
    : has_compress_gzip_(false), has_xzver_(false), is_compressed_(false)
{}

protocol::~protocol()
{}

void protocol::connect()
{
    has_compress_gzip_ = false;
    has_xzver_         = false;
    is_compressed_     = false;
    group_.clear();

    transact(nntp::cmd_welcome{});
    querycaps();
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

std::pair<bool, protocol::groupinfo> protocol::query_group(const std::string& groupname)
{
    nntp::cmd_group cmd {groupname};

    const auto code = transact(&cmd);
    if (code != nntp::cmd_group::SUCCESS)
        return {false, groupinfo{0}};

    group_ = groupname;
    return { true, 
        groupinfo {cmd.high_water_mark, cmd.low_water_mark, cmd.article_count}
    };
}

bool protocol::download_article(const std::string& article, buffer& buff)
{
    // nntp::cmd_body cmd {article, buff.capacity, buff.ptr};

    // const auto code = transact(&cmd);
    // if (code != cmd.SUCCESS)
    //     return false;

    // buff.offset = cmd.offset;
    // buff.size   = cmd.size;
    return true;
}

void protocol::download_overview(const std::string& first, const std::string& last, buffer& buff)
{
    if (has_xzver_ && !is_compressed_)
    {
        // try to turn on the compression
        const auto code = transact(nntp::cmd_enable_compress_gzip{});
        if (code == nntp::cmd_enable_compress_gzip::SUCCESS)
            is_compressed_ = true;

    }

    if (is_compressed_)
    {
        // nntp::cmd_xzver cmd {first, last, buff.capacity, buff.ptr};
        // transact(&cmd);

        // buff.offset = cmd.offset;
        // buff.size   = cmd.size;
    }
    else
    {
        // nntp::cmd_xover cmd {first, last, buff.capacity, buff.ptr};
        // transact(&cmd);

        // buff.offset = cmd.offset;
        // buff.size   = cmd.size;
    }
}

int protocol::authenticate()
{
    std::string username;
    std::string password;
    on_authenticate(username, password);

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
    return ret;
}

int protocol::querycaps()
{
    nntp::cmd_capabilities cmd;

    cmd.cmd_recv = on_recv;
    cmd.cmd_send = on_send;
    cmd.cmd_log  = on_log;

    const auto ret = cmd.transact();
    if (ret == nntp::cmd_capabilities::SUCCESS)
    {
        has_compress_gzip_ = cmd.has_compress_gzip;
        has_xzver_ = cmd.has_xzver;
    }
    return ret;
}


} // newsflash
