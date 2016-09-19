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

#if defined(WINDOWS_OS)
#  include <windows.h>
#  include <DbgHelp.h> // for minidumps
#  pragma comment(lib, "DbgHelp.lib")
#  pragma comment(lib, "User32.lib")
#endif
#include <atomic>
#include <cassert>
#include "minidump.h"

namespace seh
{

#if defined(WINDOWS_OS)

namespace {

std::atomic_flag mutex = ATOMIC_FLAG_INIT;

} // namespace

DWORD write_mini_dump(EXCEPTION_POINTERS* eptr)
{
    // if one thread is already dumping core then spin lock other threads here.
    if (mutex.test_and_set())
        while(1);

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

    // MiniDumpWithThreadInfo
    // Include thread state information. For more information, see MINIDUMP_THREAD_INFO_LIST. 
    // DbgHelp 6.1 and earlier:  This value is not supported.
    //
    // MiniDumpWithIndirectlyReferencedMemory
    // Include pages with data referenced by locals or other stack memory. This option can increase the size of the minidump file significantly. 
    // DbgHelp 5.1:  This value is not supported.
    const DWORD flags[] = {
        MiniDumpWithThreadInfo | MiniDumpWithIndirectlyReferencedMemory,
        MiniDumpWithIndirectlyReferencedMemory,        
        0
    };

    for (auto flag : flags)
    {
        const auto ret = MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            crash_log,      
            MINIDUMP_TYPE(MiniDumpNormal | flag),
            &mini,
            NULL, // UserStreamParam
            NULL);  // CallbackParam
        if (ret == TRUE)
            break;
    }

    CloseHandle(crash_log);

    // todo: we should figure out how to display this message to the user 
    // in case a non-gui thread faults. maybe need a watchdog?
    if (IsGUIThread(FALSE) == TRUE)
    {
        MessageBoxA(
            NULL,
            NEWSFLASH_TITLE " has encountered a serious error.\r\n"
            "Please help me try to fix the problem by sending a detailed description of events\r\n"
            "that lead to the crash. Please also attach the  crashdump_" NEWSFLASH_VERSION ".dmp file.\r\n"
            "In order to replicate the problem I might need to inspect your data,\r\n"
            "so please preseve your current data files as they are. This also includes\r\n"
            "your config files in your \"Documents and Settings\\username\\.newsflash folder\"\r\n"
            "Your data might be vital for resolving the problem!\r\n"
            "Send your reports with your contact details to samiv@ensisoft.com\r\n"
            "Please subject your email with \"Crashreport\"",
            NEWSFLASH_TITLE,
            MB_ICONERROR);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

#endif

} // seh

