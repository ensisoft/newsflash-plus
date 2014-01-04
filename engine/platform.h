// Copyright (c) 2013 Sami Väisänen, Ensisoft 
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

#pragma once

#include <newsflash/config.h>

#if defined(WINDOWS_OS)
#  include <windows.h>
#  include <winsock.h>
#elif defined(LINUX_OS)
#  include <sys/types.h>
#  include <cerrno>
#endif

#include <chrono>
#include <vector>
#include <string>

// this file contains an assortment of platform specific global functions
// and associated types.

namespace newsflash
{

#if defined(WINDOWS_OS)

    typedef HANDLE  handle_t;
    typedef SOCKET  socket_t;
    typedef FD_SET  fd_set;
    typedef DWORD   errcode_t;

    // the constant's have a OS_ prefix because some stuff like SOCKET_ERROR
    // is a macro *sigh* and wreaks havoc otherwise...
    const HANDLE OS_INVALID_HANDLE        = NULL;    
    const SOCKET OS_INVALID_SOCKET        = INVALID_SOCKET;

    const int OS_SOCKET_ERROR_IN_PROGRESS = WSAEWOULDBLOCK;
    const int OS_SOCKET_ERROR_RESET       = 0; // todo
    const int OS_SOCKET_ERROR_TIMEOUT     = 0; // todo
    const int OS_SOCKET_ERROR_REFUSED     = 0; // todo:
    const int OS_ERROR_NO_ACCESS          = ERROR_ACCESS_DENIED;
    const int OS_ERROR_FILE_NOT_FOUND     = ERROR_FILE_NOT_FOUND;
    const int OS_BAD_HANDLE               = ERROR_INVALID_HANDLE;

#elif defined(LINUX_OS)

    typedef int    handle_t;
    typedef int    socket_t;
    typedef fd_set fd_set;
    typedef int    errcode_t;

    const int OS_INVALID_HANDLE           = -1;
    const int OS_INVALID_SOCKET           = -1;

    const int OS_SOCKET_ERROR_IN_PROGRESS = EINPROGRESS;
    const int OS_SOCKET_ERROR_RESET       = ECONNRESET;
    const int OS_SOCKET_ERROR_TIMEOUT     = ETIMEDOUT;
    const int OS_SOCKET_ERROR_REFUSED     = ECONNREFUSED;
    const int OS_ERROR_NO_ACCESS          = EACCES;
    const int OS_ERROR_FILE_NOT_FOUND     = ENOENT;
    const int OS_BAD_HANDLE               = EBADF;

#endif

    // convenience wrapper to specify different time intervals.
    template<typename Rep, typename Period>
    void wait_single_object(handle_t handle, const std::chrono::duration<Rep, Period>& wait_duration)
    {
    // todo: convert to millisecondstype
    }

    template<typename Rep, typename Period>
    void wait_multiple_objects(const std::vector<handle_t>& handles, const std::chrono::duration<Rep, Period>& wait_duration)
    {
    // todo: convert to millisecondstype
    }

    // wait on a single handle indefinitely untill the handle is signaled.
    void wait_single_object(handle_t handle);

    // wait on a single handle untill the given ms elapses or the handle is signaled.
    // returns true if object was signaled, otherwise returns false.
    bool wait_single_object(handle_t handle, std::chrono::milliseconds ms);

    // wait on a number of handles untill one of them is signaled.
    // returns the handle that was signaled.
    handle_t wait_multiple_objects(const std::vector<handle_t>& handles);

    // wait on a number of handles untill one of them is signaled or untill
    // the given timespan elapses. 
    // returns the handle that was signaled or invalid handle on timeout
    handle_t wait_multiple_objects(const std::vector<handle_t>& handles, std::chrono::milliseconds ms);

    inline 
    bool is_signaled(handle_t handle)
    {
        // poll the handle
        return wait_single_object(handle, std::chrono::milliseconds(0)); 
    }

    // get a platform provided human readable error string.
    std::string get_error_string(int code);

    errcode_t get_last_error();

} // newsflash

