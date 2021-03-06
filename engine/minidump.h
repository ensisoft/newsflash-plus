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

#pragma once

#include <newsflash/config.h>

// SEH (structured exception handling) is a windows feature
// and the __try __except constructrs are a msvc extension
#if defined(__MSVC__)
#  include <windows.h>
#  include <dbghelp.h> // for minidumps
#  pragma comment(lib, "dbghelp.lib")

namespace seh
{
    void init_mini_dump();

    DWORD write_mini_dump(EXCEPTION_POINTERS* eptr);

} // seh

#define SEH_BLOCK(x) \
    __try { \
        x \
    } __except(seh::write_mini_dump(GetExceptionInformation())) {}

#else
  #define SEH_BLOCK(x) x
#endif