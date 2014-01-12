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

#include <stdexcept>
#include <algorithm>
#include <cassert>
#include <cerrno>
#if defined(WINDOWS_OS)
#  include <windows.h>
#  include <winsock2.h>
#elif defined(LINUX_OS)
#  include <sys/types.h>
#  include <sys/select.h>
#  include <sys/socket.h>
#  include <cstring>
#endif
#include "socketapi.h"
#include "waithandle.h"

namespace {

std::pair<bool, bool> check_read_write(newsflash::native_socket_t sock)
{
    fd_set read;
    fd_set write;

    FD_SET(sock, &read);
    FD_SET(sock, &write);

    if (select(0, &read, &write, nullptr, nullptr) == newsflash::OS_SOCKET_ERROR)
        throw std::runtime_error("select");

    bool can_read  = FD_ISSET(sock, &read);
    bool can_write = FD_ISSET(sock, &write);

    return { can_read, can_write };

}

}  // namespace

namespace newsflash
{

#if defined(WINDOWS_OS)

bool waithandle::wait_handles(const list& handles, const std::chrono::milliseconds* ms)
{
    assert(std::distance(std::begin(handles), std::end(handles)) > 0);

    std::vector<HANDLE> vec;
    for (auto& handle : handles)
    {
        vec.push_back(handle->handle_);
    }

    const DWORD start  = GetTickCount();
    const DWORD span   = ms ? ms->count() : 0;
    const DWORD nCount = static_cast<DWORD>(vec.size());
    
    DWORD now = start;
    DWORD signaled = 0;
    
    while (true)
    {      
        if ((now - start) > span)
            return false;
            
        const DWORD timeout = ms ? (span - (now - start)) : INFINITE;
      
        const DWORD ret = WaitForMultipleObjectsEx(nCount, &vec[0], FALSE, timeout, FALSE);
        if (ret == WAIT_FAILED)
            throw std::runtime_error("wait failed");
        else if (ret == WAIT_TIMEOUT)
             return false;

        signaled = ret - WAIT_OBJECT_0;

        auto handle = handles[ret - WAIT_OBJECT_0];
        if (handle->type_ == handle::type::socket)
        {
            const auto& state = check_read_write(handle->handle);
            if ((state.first && handle->read_) || (state.second && handle->write_))
            {
                handle->read_  = state.first;
                handle->write_ = state.second;
                break;
            }
        }
        else
        {
            handle->read_  = true;
            handle->write_ = false;
            break;
        }
        now = GetTickCount();
        // todo: deal with wrapround, i.e. if now < start
    }

    for (DWORD i=0; i<nCount; ++i)
    {
        if (i == signaled)
            continue;
        vec[i]->read_  = false;
        ved[i]->write_ = false;
    }
    return true;

}

#elif defined(LINUX_OS)

bool waithandle::wait_handles(const list& handles, const std::chrono::milliseconds* ms)
{
    assert(std::distance(std::begin(handles), std::end(handles)) > 0);    

    fd_set read;
    fd_set write;
    FD_ZERO(&read);
    FD_ZERO(&write);

    int maxfd = 0;

    for (const auto& handle : handles)
    {
        if (handle->read_)
            FD_SET(handle->handle_, &read);
        if (handle->write_)
            FD_SET(handle->handle_, &write);
        maxfd = std::max(handle->handle_, maxfd);
    }

    if (ms)
    {
        struct timeval tv {0};        
        tv.tv_usec = ms->count() * 1000;
        
        const int ret = select(maxfd + 1, &read, &write, nullptr, &tv);
        if (ret == -1)
            throw std::runtime_error("wait failed");
        else if (ret == 0)
        {
            for (auto& handle : handles)
            {
                handle->write_ = false;
                handle->read_  = false;
            }
            return false;
        }
    }
    else
    {
        if (select(maxfd + 1, &read, &write, nullptr, nullptr) == -1)
            throw std::runtime_error("wait failed");
    }

    for (auto& handle : handles)
    {
        if (handle->read_)
        {
            if (!FD_ISSET(handle->handle_, &read))
                handle->read_ = false;
        }
        if (handle->write_)
        {
            if (!FD_ISSET(handle->handle_, &write))
                handle->write_ = false;
        }
    }
    return true;
}

#endif

} // newsflash



