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

namespace newsflash
{

localtime get_localtime()
{
    localtime ret = {0};
#if defined(LINUX_OS)
    struct timeval tv;
    // get seconds and microseconds
    gettimeofday(&tv, NULL);
    
    tm time;
    localtime_r(&tv.tv_sec, &time);
    
    ret.hours   = time.tm_hour;
    ret.minutes = time.tm_min;
    ret.seconds = time.tm_sec;
    ret.millis  = tv.tv_usec / 1000;
#elif defined(WINDOWS_OS)
    SYSTEMTIME sys;
    GetLocalTime(&sys);
    ret.hours   = sys.wHour;
    ret.minutes = sys.wMinute;
    ret.seconds = sys.wSecond;
    ret.millis  = sys.wMilliseconds;    

#endif
    return ret;

}

unsigned long get_thread_identity()
{
#if defined(LINUX_OS)
    return (unsigned long)pthread_self();
#elif defined(WINDOWS_OS)
    return (unsigned long)GetCurrentThreadId();    
#endif
}

} // newsflash

