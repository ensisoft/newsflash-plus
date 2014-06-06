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

// these are macros so that they can be embedded
// inside string literals easily for exaple L"yadi yadi" VERSION_STRING "yadi yadi"
#define NEWSFLASH_VERSION   "3.5.0b0"

#define NEWSFLASH_COPYRIGHT "Copyright (c) Sami V\303\244is\303\244nen 2009-2014"
#define NEWSFLASH_WEBSITE   "http://www.ensisoft.com"

#ifndef _NDEBUG
#  define NEWSFLASH_TITLE "Newsflash Plus"
#else 
#  define NEWSFLASH_TITLE "Newsflash Plus DEBUG"
#endif