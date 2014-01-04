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

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <atomic>
#if defined(WINDOWS_OS)
#  include <windows.h>
#  include <DbgHelp.h> // for minidumps
#  pragma comment(lib, "DbgHelp.lib")
#elif defined(LINUX_OS)
#  include <execinfo.h>
#  include <sys/wait.h>
#  include <sys/ptrace.h>
#  include <unistd.h>
#endif


// So where is my core file? 
// check that core file is unlimited
// ulimit -c unlimited
// check the kernel core pattern
// cat /proc/sys/kernel/core_pattern
//
// Some distros such as ArchLinux use systemd to 
// store the core files in the journal. these can be retrieved
// with systemd-coredumpctl

std::atomic_flag mutex;


namespace newsflash
{

void do_assert(const char* expression, const char* file, const char* func, int line)
{
    // if one thread is already asserting then spin lock other threads here.
    if (mutex.test_and_set())
        while (1);


    std::fprintf(stderr, "%s:%i: %s: Assertion `%s' failed.\n", file, line, func, expression);

    // for windows XP and Windows Server 2003 FramestoSkip and FramesToCapture
    // must be less than 63.
    const int MAX_CALLSTACK = 62; 

#if defined(WINDOWS_OS)
    // todo: use StackWalk64 to print stack frame

    const HANDLE process = GetCurrentProcess();

    char filename[100] = {0};
    sprintf(filename, "newsflash-core.%d", process);
    
    // write minidump
    HANDLE handle = CreateFileA(filename,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (handle != INVALID_HANDLE_VALUE)
    {
        MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            handle,
            MiniDumpNormal,
            nullptr, // exception info
            nullptr, // UserStreamParam
            nullptr); // callbackParam
        CloseHandle(handle);
    }
#elif defined(LINUX_OS)
    void* callstack[MAX_CALLSTACK] = {0};

    const int frames = backtrace(callstack, MAX_CALLSTACK);
    if (frames == 0)
        std::abort();

    // for backtrace symbols to work you need -rdynamic ld (linker) flag
    char** strings = backtrace_symbols(callstack, frames);
    if (strings == nullptr)
        std::abort();

    // todo: demangle
    for (int i=0; i<frames; ++i)
    {
        std::fprintf(stderr, "Frame (%d): @ %p, '%s'\n", 
            i, callstack[i], strings[i]);
    }

    free(strings);

#endif
    std::abort();
}

bool has_debugger()
{
#if defined(WINDOWS_OS)
    return IsDebuggerPresent();
#elif defined(LINUX_OS)
    // this doesn't work reliably, so we just do the same
    // as standard assert() does on linux, simply abort


    // int fds[2];
    // if (pipe(fds) == -1)
    //     std::abort(); // horrors

    // const pid_t child = fork();
    // if (child == -1)
    //     std::abort(); // horrors

    // if(child == 0)
    // {
    //     int we_have_tracer = 1;

    //     const pid_t parent = getppid();

    //     // try attaching to the parent process. if this works
    //     // then nobody is tracing the parent, hence it cannot
    //     // be debugged either.
    //     if (ptrace(PTRACE_ATTACH, parent, NULL, NULL))
    //     {
    //         waitpid(parent, NULL, 0);
    //         ptrace(PTRACE_CONT, NULL, NULL);

    //         // detach
    //         ptrace(PTRACE_DETACH, parent, NULL, NULL);

    //         we_have_tracer = 0;
    //     }
    //     // communicate result back to the parent.
    //     // note that we can't directly use the exit status code
    //     // since a non-zero exit code will cause problems with tools
    //     // such as boost.unit test.
    //     write(fds[1], &we_have_tracer, sizeof(int));

    //     _exit(0);
    // }

    // // the parent will wait for the result from the child
    // // checking if the tracer succeeds or not.
    // int status = 0;

    // waitpid(child, &status, 0);

    // int we_have_tracer = 0;
    // read(fds[0], &we_have_tracer, sizeof(int));

    // printf("we_have_tracer:%d\n", we_have_tracer);

    // close(fds[0]);
    // close(fds[1]);

    // return we_have_tracer == 1;

    return false;

#endif
}

} // newsflash