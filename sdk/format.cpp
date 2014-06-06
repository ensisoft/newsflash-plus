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

#if defined(WINDOWS_OS)
#  include <windows.h>
#elif defined(LINUX_OS)
#  include <langinfo.h> // for nl_langinfo
#endif

#include <cstring>
#include "format.h"

namespace sdk
{

QString widen(const char* str)
{
#if defined(WINDOWS_OS)
    return QString::fromLatin1(str);

#elif defined(LINUX_OS)
    const char* codeset = nl_langinfo(CODESET);
    if (!std::strcmp(codeset, "UTF-8"))
        return QString::fromUtf8(str);
    return QString::fromLocal8Bit(str);
#endif
}

QString widen(const wchar_t* str)
{
    // windows uses UTF-16 (UCS-2 with surrogate pairs for non BMP characters)    
    // if this gives a compiler error it means that wchar_t is treated
    // as a native type by msvc. In order to compile wchar_t needs to be
    // unsigned short. Doing a cast will help but then a linker error will 
    // follow since Qt build assumes that wchar_t = unsigned short
#if defined(WINDOWS_OS)
    return QString::fromUtf16(str);
#elif defined(LINUX_OS)
    static_assert(sizeof(wchar_t) == sizeof(uint), "");

    return QString::fromUcs4((const uint*)str);
#endif
}

} // sdk