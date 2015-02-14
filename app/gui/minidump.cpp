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

#include <newsflash/config.h>

#include <cassert>
#include "minidump.h"
#include "config.h"

namespace gui
{

#if defined(WINDOWS_OS)

DWORD writeMiniDump(EXCEPTION_POINTERS* eptr)
{
    HANDLE crash_log = CreateFileA(
        "crashdump_" NEWSFLASH_VERSION ".dmp",
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (crash_log == INVALID_HANDLE_VALUE)
        return EXCEPTION_CONTINUE_SEARCH;

    MINIDUMP_EXCEPTION_INFORMATION mini;
    memset(&mini, 0, sizeof(MINIDUMP_EXCEPTION_INFORMATION));
    mini.ThreadId = GetCurrentThreadId();
    mini.ExceptionPointers = eptr; // should this be marshalled?
    mini.ClientPointers = FALSE;


    const HANDLE proc = GetCurrentProcess();
    const DWORD  pid  = GetCurrentProcessId();
    BOOL   ret  = MiniDumpWriteDump(
        proc,
        pid,
        crash_log,      
        MiniDumpNormal, // | MiniDumpWithThreadInfo,
        &mini,
        NULL, // UserStreamParam
        NULL  // CallbackParam
        );
    assert( ret == TRUE );
    CloseHandle(crash_log);
    MessageBoxA(
        NULL,
        "Newsflash has encountered a serious error.\r\n"
        "Please help me try to fix the problem by sending a detailed description of events\r\n"
        "that lead to the crash. Please also attach the  crashdump_" NEWSFLASH_VERSION ".dmp file.\r\n"
        "In order to replicate the problem I might need to inspect your data,\r\n"
        "so please preseve your current data files as they are. This also includes\r\n"
        "your config files in your \"Documents and Settings\\username\\.newsflash folder\"\r\n"
        "Your data might be vital for resolving the problem!\r\n"
        "Send your reports with your contact details to samiv@ensisoft.com\r\n"
        "Please subject your email with \"Crashreport\"",
        "Newsflash",
        MB_ICONERROR);

    return EXCEPTION_CONTINUE_SEARCH;
}

#endif

} // gui

