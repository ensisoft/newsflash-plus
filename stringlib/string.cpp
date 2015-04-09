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

// $Id: string.cpp,v 1.3 2009/11/15 14:56:36 svaisane Exp $

#include <algorithm>
#include <cctype>
#include "string.h"

namespace {

struct finder {
    finder(int c, bool case_sensitive) : char_(c), case_sensitive_(case_sensitive)
    {
        if (!case_sensitive_)
            char_ = std::toupper(char_);
    }
    bool operator()(int c) const
    {
        return case_sensitive_ ? 
          char_ == c :
          std::toupper(c) == char_;
    }
    int char_;
    bool case_sensitive_;
};

} // namespace

namespace stringlib
{

std::string find_longest_common_substring(std::vector<std::string> vec, bool case_sensitive)
{
    if (vec.empty())
        return "";
    if (vec.size() == 1)
        return vec[0];

    std::sort(vec.begin(), vec.end(), [](const std::string& lhs, const std::string& rhs) {
        return lhs.size() < rhs.size();
    });

    std::string substring = vec[0];
    for (std::vector<std::string>::size_type i(0); i<vec.size(); ++i)
    {
        substring = find_longest_common_substring(vec[i], substring, case_sensitive);
    }
    return substring;
}    

std::string find_longest_common_substring(const std::string& master, const std::string& slave, bool case_sensitive)
{
    std::string str;

    for (std::string::size_type i(0); i<master.size(); ++i)
    {
        std::string tmp;
        std::string::size_type pos = 0;
        while (pos != std::string::npos)
        {
            std::string::const_iterator it = slave.begin();
            std::advance(it, pos);
            it = std::find_if(it, slave.end(), finder(master[i], case_sensitive));
            if (it == slave.end())
                break;
            
            pos = std::distance(slave.begin(), it);

            std::string::size_type x = i;
            while (true)
            {
                if (case_sensitive)
                {
                    if (master[x] != slave[pos])
                        break;
                }
                else
                {
                    if (std::toupper(master[x]) != std::toupper(slave[pos]))
                        break;
                }
                tmp += master[x];
                if (++pos == slave.size())
                    break;
                if (++x == master.size())
                    break;
            }
            if(tmp.size() > str.size())
                str = tmp;

            tmp.clear();
        }
    }
    return str;
}

} // stringlib
