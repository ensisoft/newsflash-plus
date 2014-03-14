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

#include <vector>
#include <string>

namespace std
{
    // adapters functions for contiguous std types that can be used as a buffer.
    
    inline 
    size_t buffer_capacity(const std::string& str) 
    { 
        return str.size(); 
    }
    inline 
    void* buffer_data(std::string& str) 
    { 
        return &str[0]; 
    }
    inline 
    void  grow_buffer(std::string& str, size_t capacity) 
    { 
        str.resize(capacity); 
    }

    template<typename T> inline
    size_t buffer_capacity(const std::vector<T>& vec) 
    { 
        return vec.size() * sizeof(T); 
    }

    template<typename T> inline
    void* buffer_data(std::vector<T>& vec) 
    { 
        return &vec[0]; 
    }

    template<typename T> inline
    void grow_buffer(std::vector<T>& vec, size_t capacity) 
    { 
        const size_t items = (capacity + sizeof(T) - 1) / sizeof(T);
        vec.resize(items);
    }


} // std
