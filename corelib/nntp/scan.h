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

#include <boost/algorithm/string/trim.hpp>
#include <sstream>
#include <string>
#include <iomanip>

namespace nntp
{
    struct trailing_comment {
        std::string str;
    };

    namespace detail {
        template<typename T>
        void extract_value(std::stringstream& ss, T& value)
        {
            ss >> std::skipws >> value;
        }

        inline
        void extract_value(std::stringstream& ss, std::string& str)
        {
            ss >> std::skipws >> str;
        }

        inline
        void extract_value(std::stringstream& ss, trailing_comment& comment)
        {
            std::getline(ss, comment.str);
            boost::algorithm::trim(comment.str);
        }

        template<typename Value>
        void scan_next_value(std::stringstream& ss, Value& next)
        {
            extract_value(ss, next);
        }

        template<typename Value, typename... Rest>
        void scan_next_value(std::stringstream& ss, Value& next, Rest&... gang)
        {
            extract_value(ss, next);
            scan_next_value(ss, gang...);
        }

    } // detail


    // scan response string for a sequence of values given their specific type. 
    // returns true if scan was succesful, otherwise false.
    template<typename... Values>
    bool scan_response(const std::string& response, Values&... values)
    {
        std::stringstream ss(response);
        detail::scan_next_value(ss, values...);
        return !ss.fail();
    }


} // nntp
