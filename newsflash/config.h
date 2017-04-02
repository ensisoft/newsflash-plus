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

// software configuration options
// msvc for windows, (g++) or clang for linux

#ifdef _MSC_VER
  #define WINDOWS_OS
  #define __MSVC__
  #ifdef _M_AMD64
    #define X86_64
  #endif

  #define COMPILER_NAME    "msvc"
  #define COMPILER_VERSION _MSC_FULL_VER

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

  //#define _ITERATOR_DEBUG_LEVEL 2

  // on windows use UTF8 in the engine api to pass strings
  // and then use Unicode win32 API
  #define UNICODE

  // wingdi.h defines error. so do we in logging.h
  #undef ERROR

  // msvc2013 doesn't support noexcept.
  // todo: is this smart, the semantics are not *exactly* the same...
  #define  NOTHROW throw()

  #define NORETURN __declspec(noreturn)
#elif defined(__clang__)
  #ifdef __LP64__
    #define X86_64
  #endif
  #define __CLANG__
  #define LINUX_OS

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

  #define COMPILER_NAME    "clang"
  #define COMPILER_VERSION __clang_version__
  #define NOTHROW  noexcept
  #define NORETURN [[noreturn]]
#elif defined(__GNUG__)
  #ifdef __LP64__
    #define X86_64
  #endif
  #define __GCC__
  #define LINUX_OS
  #define COMPILER_NAME "GCC"
  #define COMPILER_VERSION __GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__
  #define NOTHROW  noexcept
  #define NORETURN [[noreturn]]
#endif

#ifndef NDEBUG
#  define NEWSFLASH_DEBUG
#  undef QT_NO_DEBUG
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
#define NEWSFLASH_VERSION   "4.1.1"
//#define NEWSFLASH_VERSION   "master" // remember to edit!
#define NEWSFLASH_COPYRIGHT "Copyright (c) Sami V\303\244is\303\244nen 2005-2017"
#define NEWSFLASH_WEBSITE   "http://www.ensisoft.com"

#ifdef NDEBUG
#  define NEWSFLASH_TITLE "Newsflash Plus"
#else
#  define NEWSFLASH_TITLE "Newsflash Plus DEBUG"
#endif

#ifdef X86_64
#  define NEWSFLASH_ARCH "64bit"
#else
#  define NEWSFLASH_ARCH "32bit"
#endif
//#define NEWSFLASH_DEBUG