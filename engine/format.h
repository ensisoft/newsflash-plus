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

#include <string>
#include <sstream>
#include <stdexcept>

namespace newsflash
{
    namespace detail {

        template<typename T>
        void format_next(std::stringstream& ss, const T& value)
        {
            ss << value;
        }

        template<typename T, typename... Rest>
        void format_next(std::stringstream& ss, const T& value, const Rest&... rest)
        {
            ss << value;
            format_next(ss, rest...);
        }

    } // detail

template<typename... Args>
std::string format(const Args&... args)
{
    std::stringstream ss;
    if (!ss.good())
        throw std::runtime_error("string format error");

    detail::format_next(ss, args...);

    return ss.str();
}


} // newsflash
