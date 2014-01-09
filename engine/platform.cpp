// Copyright (c) 2010-2013 Sami Väisänen, Ensisoft 
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

#include <atomic>
#include <stdexcept>
#include <algorithm>
#include <cassert>
#include <cerrno>
#if defined(LINUX_OS)
#  include <sys/types.h>
#  include <sys/select.h>
#  include <sys/socket.h>
#  include <cstring>
#endif
#include "platform.h"

namespace newsflash
{

#if defined(WINDOWS_OS)

void wait_single_object(native_handle_t handle)
{
    const DWORD ret = WaitForSingleObject(handle, INFINITE);
    if (ret != WAIT_OBJECT_0)
        throw std::runtime_error("wait failed (WaitForSingleObject");
}

bool wait_single_object(native_handle_t handle, const std::chrono::milliseconds& ms)
{
    const DWORD ret = WaitForSingleObject(handle, ms.count());
    if (ret == WAIT_FAILED || ret == WAIT_ABANDONED)
        throw std::runtime_error("wait failed (WaitForSingleObject");

    return (ret == WAIT_TIMEOUT) ? false : true;
}

native_handle_t wait_multiple_objects(const std::vector<native_handle_t>& handles)
{
    const DWORD nCount = (DWORD)handles.size();
    assert(nCount);

    const DWORD ret = WaitForMultipleObjectsEx(nCount, &handles[0], FALSE, INFINITE, FALSE);
    if (ret >= WAIT_OBJECT_0 && ret <= (nCount - 1))
        return handles.at(ret - WAIT_OBJECT_0);

    throw std::runtime_error("wait failed (WaitForMultipleObjectsEx)");
}

native_handle_t wait_multiple_objects(const std::vector<native_handle_t>& handles, const std::chrono::milliseconds& ms)
{
    const DWORD dwMillis = ms.count();
    const DWORD nCount = (DWORD)handles.size();
    assert(nCount);

    const DWORD ret = WaitForMultipleObjectsEx(nCount, &handles[0], FALSE, dwMillis, FALSE);
    if (ret == WAIT_TIMEOUT)
        return OS_INVALID_HANDLE;
    else if (ret >= WAIT_OBJECT_0 && ret <= (nCount -1))
        return handles.at(ret - WAIT_OBJECT_0);

    throw std::runtime_error("wait failed (WaitForMultipleObjectsEx)");
}

std::string get_error_string(int code)
{
    //const LCID language = GetUserDefaultLCID();

    char* buff = nullptr;
    const DWORD ret = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM  |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        code,
        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
        (LPSTR)&buff, 
        0,
        NULL);
    if (ret == 0 || !buff)
        throw std::runtime_error("format message failed");

    std::string str(buff, strlen(buff)-2); // erase the new line added by FormatMessage
    LocalFree(buff);
    return str;    
}

errcode_t get_last_error()
{
    return GetLastError();
}

#elif defined(LINUX_OS)

bool is_socket(native_handle_t handle)
{
    int type = 0;
    socklen_t len = sizeof(type);
    if (!getsockopt(handle, SOL_SOCKET, SO_TYPE, &type, &len))
        return true;

    assert(errno == ENOTSOCK);
    return false;
}


void wait_single_object(native_handle_t handle)
{
    fd_set read;
    fd_set write;

    FD_ZERO(&read);
    FD_ZERO(&write);
    FD_SET(handle, &read);
    //FD_SET(handle, &write);

    if (::select(handle + 1, &read, &write, nullptr, nullptr) == -1)
        throw std::runtime_error("wait failed (select)");
}

bool wait_single_object(native_handle_t handle, const std::chrono::milliseconds& ms)
{
    fd_set read;
    fd_set write;

    FD_ZERO(&read);
    FD_ZERO(&write);
    FD_SET(handle, &read);
    //FD_SET(handle, &write);

    struct timeval tv {0};
    tv.tv_usec = ms.count() * 1000;

    if (::select(handle + 1, &read, &write, nullptr, &tv) == -1)
        throw std::runtime_error("wait failed (select)");

    if (FD_ISSET(handle, &read) || FD_ISSET(handle, &write))
        return true;

    return false;
}

native_handle_t wait_multiple_objects(const std::vector<native_handle_t>& handles)
{
    fd_set read;
    fd_set write;

    int max_fd = 0;

    FD_ZERO(&read);
    FD_ZERO(&write);
    for (const auto handle : handles)
    {
        FD_SET(handle, &read);
        //FD_SET(handle, &write);
        max_fd = std::max(max_fd, handle);
    }

    if (::select(max_fd + 1, &read, &write, nullptr, nullptr) == -1)
        throw std::runtime_error("wait failed (select)");

    for (const auto handle : handles)
    {
        if (FD_ISSET(handle, &read) || FD_ISSET(handle, &write))
            return handle;
    }
    assert(0);
}

native_handle_t wait_multiple_objects(const std::vector<native_handle_t>& handles, const std::chrono::milliseconds& ms)
{
    fd_set read;
    fd_set write;

    int max_fd = 0;

    FD_ZERO(&read);
    FD_ZERO(&write);
    for (const auto handle : handles)
    {
        FD_SET(handle, &read);
        //FD_SET(handle, &write);
        max_fd = std::max(max_fd, handle);
    }    

    struct timeval tv {0};
    tv.tv_usec = ms.count() * 1000;

    if (::select(max_fd + 1, &read, &write, nullptr, &tv) == -1)
        throw std::runtime_error("wait failed (select)");

    for (const auto handle : handles)
    {
        if (FD_ISSET(handle, &read) || FD_ISSET(handle, &write))
            return handle;
    }
    return OS_INVALID_HANDLE;
}

std::string get_error_string(int code)
{
    char buff[128] = {0};
    return std::string(strerror_r(code, buff, sizeof(buff)));
}

native_errcode_t get_last_error() 
{
    return errno;
}

#endif

} // newsflash

