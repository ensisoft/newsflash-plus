// Copyright (c) 2014 Sami Väisänen, Ensisoft 
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

#include <newsflash/config.h>

#include <string>
#include <sstream>
#include <stdexcept>
#include <cstdint>
#include "utility.h"
#include "types.h"

namespace newsflash
{
    inline
    void str(std::ostream& ss, const newsflash::ipv4& ip)
    {
        ss << ((ip.addr >> 24) & 0xff) << "."
           << ((ip.addr >> 16) & 0xff) << "."
           << ((ip.addr >> 8) & 0xff) << "."
           << (ip.addr & 0xff);
    }

    inline
    void str(std::ostream& ss, const newsflash::kb& kb)
    {
        ss << kb.value / 1024.0 << " Kib";
    }

    inline
    void str(std::ostream& ss, const newsflash::mb& mb)
    {
        ss << mb.value / (1024.0 * 1024.0) << " Mib";
    }

    inline
    void str(std::ostream& ss, const newsflash::gb& gb)
    {
        ss << gb.value / (1024.0 * 1024.0 * 1024.0) << " Gib";
    }

    inline
    void str(std::ostream& ss, const newsflash::size& s)
    {
        if (s.value >= GB(1))
            str(ss, gb{s.value});
        else if (s.value >= MB(1))
            str(ss, mb{s.value});
        else str(ss, kb{s.value});
    }

    inline 
    void str(std::ostream& ss, bool val)
    {
        ss << (val ? "True" : "False");
    }


    template<typename T>
    void str(std::ostream& ss, const T& value)
    {
        ss << value;
    }

    template<typename T, typename... Rest>
    void str(std::ostream& ss, const T& value, const Rest&... rest)
    {
        ss << value;
        str(ss, rest...);
    }

    template<typename... Args>
    std::string str(const Args&... args)
    {
        std::stringstream ss;
        if (!ss.good())
            throw std::runtime_error("string format error");

        str(ss, args...);
        return ss.str();
    }

} // newsflash
