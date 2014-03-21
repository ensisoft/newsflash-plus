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

#include <system_error>

#if defined(WINDOWS_OS)
#  include <windows.h>
#  include "utf8.h"
#elif defined(LINUX_OS)
#  include <sys/time.h>
#  include <time.h>
#  include <pthread.h>
#endif
#include <stdexcept>
#include <cstring>
#include <cerrno>
#include <fstream>

#include "platform.h"


namespace engine
{

#if defined(WINDOWS_OS)

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


localtime get_localtime()
{
    localtime ret {0};

    SYSTEMTIME sys;
    GetLocalTime(&sys);
    ret.hours   = sys.wHour;
    ret.minutes = sys.wMinute;
    ret.seconds = sys.wSecond;
    ret.millis  = sys.wMilliseconds;    

    return ret;
}

unsigned long get_thread_identity()
{
    return (unsigned long)GetCurrentThreadId();
}

std::ofstream& open_fstream(const std::string &filename, std::ofstream &stream)
{
    const std::wstring& wide = utf8::decode(filename);

    stream.open(wide);
    return stream;
}

#elif defined(LINUX_OS)

std::string get_error_string(int code)
{
    char buff[128] = {0};
    return std::string(strerror_r(code, buff, sizeof(buff)));
}


localtime get_localtime()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    
    struct tm time;
    localtime_r(&tv.tv_sec, &time);
    
    localtime ret {0};

    ret.hours   = time.tm_hour;
    ret.minutes = time.tm_min;
    ret.seconds = time.tm_sec;
    ret.millis  = tv.tv_usec / 1000;    
    return ret;
}

unsigned long get_thread_identity()
{
    return (unsigned long)pthread_self();
}

std::ofstream& open_fstream(const std::string& filename, std::ofstream& stream)
{
    stream.open(filename);
    return stream;
}

#endif


} // engine

