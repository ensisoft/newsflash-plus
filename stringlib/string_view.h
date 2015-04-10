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

#include <cstddef>
#include <cstring>

namespace str
{
    class string_view
    {
    public:
        string_view() : m_str(nullptr), m_len(0)
        {}

        string_view(const char* str, std::size_t len) : m_str(str), m_len(len)
        {}

        string_view(const char* str) : m_str(str), m_len(std::strlen(str))
        {}

        std::size_t size() const 
        { return m_len; }

        using iterator = const char*;
        using const_iterator = const char*;

        iterator begin()
        { 
            return m_str;
        }
        iterator end() 
        {
            return m_str + m_len;
        }

        const_iterator begin() const 
        {
            return m_str;
        }

        const_iterator end() const 
        {
            return m_str + m_len;
        }
        void clear()
        {
            m_str = nullptr;
            m_len = 0;
        }

        const char* c_str() const 
        {
            return m_str;
        }

        void set_max_len(std::size_t len)
        {
            if (len > m_len)
                m_len = len;
        }

    private:
        const char* m_str;
        std::size_t m_len;
    };


    inline
    bool operator==(const string_view& lhs, const string_view& rhs)
    {
        if (lhs.size() != rhs.size()) 
            return false;
        return std::strncmp(lhs.c_str(), rhs.c_str(), lhs.size()) == 0;
    }

} // str