// Copyright (c) 2005-2015 Sami Väisänen, Ensisoft 
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

#include <string>
#include <sstream>

namespace python
{
    namespace detail {
        inline const char* fmt(const char*) { return "s"; }
        inline const char* fmt(const std::string&) { return "s"; }
        inline const char* fmt(const std::wstring&) { return "u"; }
        inline const char* fmt(int) { return "i"; }
        inline const char* fmt(long) { return "l"; }
        inline const char* fmt(unsigned int) { return "I"; }
        inline const char* fmt(unsigned long) { return "k"; }
        inline const char* fmt(float) { return "f"; }
        inline const char* fmt(double) { return "d"; }

        template<typename T>
        void fmt_value(std::stringstream& ss, T value)
        {
            ss << fmt(value);
        }

        template<typename T, typename... Rest>
        void fmt_value(std::stringstream& ss, T value, Rest... rest)
        {
            ss << fmt(value);
            fmt_value(ss, rest...);
        }

    } // detail

    template<typename... Args>
    std::string fmt(Args... args)
    {
        std::stringstream ss;
        detail::fmt_value(ss, args...);
        return ss.str();
    }

} // python