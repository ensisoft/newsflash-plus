// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
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

#if !defined(WINDOWS_OS)
#  error this file is only for windows
#endif
#include <windows.h>
#include <winsock.h>

namespace newsflash
{
    typedef HANDLE  native_handle_t;
    typedef SOCKET  native_socket_t;
    typedef DWORD   native_errcode_t;    
    typedef FD_SET  fd_set;

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


} // newsflash
