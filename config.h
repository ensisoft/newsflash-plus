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

#pragma once

// software configuration options
// msvc for windows, (g++) or clang for linux

#ifdef _MSC_VER

  #define WINDOWS_OS
  #define __MSVC__
  #ifdef _M_AMD64
    #define X86_64
  #endif
 
  #define COMPILER "msvc"
  #define COMPILER_VERSION "todo"

  // minimum supported version, windows Server 2003 and Windows XP
  #define _WIN32_WINNT 0x0501 

  // exclude some stuff we don't need
  #define WIN32_LEAN_AND_MEAN 

  #define __PRETTY_FUNCTION__ __FUNCSIG__

  // disable the silly min,max macros 
  #define NOMINMAX 

  // disable strcpy etc. warnings
  #define _CRT_SECURE_NO_WARNINGS
  #define _SCL_SECURE_NO_WARNINGS 

  // on windows use UTF8 in the engine api to pass strings
  // and then use Unicode win32 API
  #define UNICODE

  // msvc2013 doesn't support noexcept. 
  // todo: is this smart, the semantics are not *exactly* the same...
  #define  NOTHROW throw() 

#elif defined(__clang__)

  #define __CLANG__

  #define LINUX_OS

  #ifdef __LP64__
    #define X86_64
  #endif

  #define NOTHROW noexcept

  #if defined(SUBLIME_CLANG_WORKAROUNDS)
    // clang has a problem with gcc 4.9.0 stdlib and it complains
    // about max_align_t in cstddef (pulls it into std:: from global scope)
    // however the problem is only affecting sublimeclang in SublimeText
    // 
    // https://bugs.archlinux.org/task/40229
    // http://reviews.llvm.org/rL201729
    //
    // As a workaround we define some type with a matching typename
    // in the global namespace. 
    // the macro is enabled in 'newsflash.sublime-project'
    typedef int max_align_t;
  #endif

  #define COMPILER_NAME "clang"
  #define COMPILER_VERSION "todo"


#elif defined(__GNUG__)

  #define __GCC__

  #define LINUX_OS

  #ifdef __LP64__
    #define X86_64
  #endif

  #define COMPILER_NAME "GCC"
  #define COMPILER_VERSION __GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__

  #define NOTHROW noexcept
#endif


#ifndef NDEBUG
#  define NEWSFLASH_DEBUG
#endif


#if defined(LINUX_OS)
#  ifndef _LARGEFILE64_SOURCE
#    define _LARGEFILE64_SOURCE
#  endif

//#  ifndef _POSIX_SOURCE
//#    define _POSIX_SOURCE
//#  endif
//#  ifndef _REENTRANT
//#    define _REENTRANT
//#  endif
#  ifdef NEWSFLASH_DEBUG
//#    define _FORTIFY_SOURCE 1
#  endif
#endif

#define NEWSFLASH_ENABLE_LOG
#define NEWSFLASH_ENABLE_PYTHON

#ifndef NEWSFLASH_DEBUG
//#  define QT_NO_DEBUG
#endif

#define QT_NO_CAST_TO_ASCII

// these are macros so that they can be embedded
// inside string literals easily for exaple L"yadi yadi" VERSION_STRING "yadi yadi"
#define NEWSFLASH_VERSION   "3.5.0b0"

#define NEWSFLASH_COPYRIGHT "Copyright (c) Sami V\303\244is\303\244nen 2005-2014"
#define NEWSFLASH_WEBSITE   "http://www.ensisoft.com"

#ifdef NDEBUG
#  define NEWSFLASH_TITLE "Newsflash Plus"
#else 
#  define NEWSFLASH_TITLE "Newsflash Plus DEBUG"
#endif  