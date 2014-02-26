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

#include <newsflash/config.h>

#include <boost/filesystem/path.hpp>
#include <sstream>
#include <algorithm>
#include "filesys.h"

namespace {

#if defined(WINDOWS_OS)
bool is_illegal_filename_char(int c)
{
    swtich (c)
    {
        case '\\':
        case '/':
        case ':':
        case '*':
        case '?':
        case '"':
        case '<':
        case '>':
        case '|':
        case '^': // FAT limitation
        return true;
    }
    return false;
}

#elif defined(LINUX_OS)
bool is_illegal_filename_char(int c)
{
    switch (c)
    {
        case '/':
        case '"':
        case '<':
        case '>':
        case '|':
        case '!':
        return true;
    }
    return false;
}
bool is_illegal_filepath_char(int c)
{
    if (c == '/' || c == '\\')
        return false;

    return is_illegal_filename_char(c);
}

#endif

} // namespace


namespace fs
{

std::string remove_illegal_filename_chars(std::string name)
{
    const auto it = std::remove_if(name.begin(), name.end(), 
        is_illegal_filename_char);
    name.erase(it, name.end());
    return name;
}

std::string remove_illegal_filepath_chars(std::string name)
{
    const auto it = std::remove_if(name.begin(), name.end(),
        is_illegal_filepath_char);
    name.erase(it, name.end());
    return name;
}

std::string join_path(const std::string& a, const std::string& b)
{
    if (a.empty())
        return b;
    boost::filesystem::path path(a);
    path /= b;

    return path.string();
}

std::string name_file(int attempt, const std::string& name)
{
    if (attempt == 0)
        return name;

    std::stringstream ss;
    ss << "(" << attempt << ") " << name;
    return ss.str();
}

} // fs