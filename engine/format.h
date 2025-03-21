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

#include <string>
#include <sstream>
#include <stdexcept>
#include <cstdint>
#include "utility.h"
#include "types.h"

namespace newsflash
{
    inline
    std::ostream& operator<<(std::ostream& ss, const newsflash::ipv4& ip)
    {
        ss << ((ip.addr >> 24) & 0xff) << "."
           << ((ip.addr >> 16) & 0xff) << "."
           << ((ip.addr >> 8) & 0xff) << "."
           << (ip.addr & 0xff);
        return ss;
    }

    inline
    std::ostream& operator<<(std::ostream& ss, const newsflash::kb& kb)
    {
        ss << kb.value / 1024.0 << " Kib";
        return ss;
    }

    inline
    std::ostream& operator<<(std::ostream& ss, const newsflash::mb& mb)
    {
        ss << mb.value / (1024.0 * 1024.0) << " Mib";
        return ss;
    }

    inline
    std::ostream& operator<<(std::ostream& ss, const newsflash::gb& gb)
    {
        ss << gb.value / (1024.0 * 1024.0 * 1024.0) << " Gib";
        return ss;
    }

    inline
    std::ostream& operator<<(std::ostream& ss, const newsflash::size& s)
    {
        if (s.value >= GB(1))
            ss << gb{s.value};
        else if (s.value >= MB(1))
            ss << mb{s.value};
        else ss << kb{s.value};

        return ss;
    }

    inline
    std::ostream& operator<<(std::ostream& ss, bool val)
    {
        ss << (val ? "True" : "False");
        return ss;
    }

    namespace detail {
        template<typename T>
        void str(std::stringstream& ss, const T& value)
        {
            ss << value;
        }

        template<typename T, typename... Rest>
        void str(std::stringstream& ss, const T& value, const Rest&... rest)
        {
            ss << value;
            detail::str(ss, rest...);
        }
    } // detail

    template<typename... Args>
    std::string str(const Args&... args)
    {
        std::stringstream ss;
        detail::str(ss, args...);
        if (!ss.good())
            throw std::runtime_error("string format error");
        
        return ss.str();
    }

} // newsflash
