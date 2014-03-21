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

// boost copyright here

#pragma once

#include <limits>
#include <cstddef>

namespace engine
{
    // hashing code ripped off from boost TR1
    template<typename T>
    class hash
    {
    public:
        hash() : seed_(0) {}

        inline void add(const T& val)
        {
            seed_ ^= hash_value_signed(val) + 0x9e3779b9 + (seed_<<6) + (seed_>>2);
        }
        inline std::size_t result() const
        {
            return seed_;
        }
    private:
        std::size_t hash_value_signed(const T& val) const
        {
            const int size_t_bits = std::numeric_limits<std::size_t>::digits;
            const int length = (std::numeric_limits<T>::digits - 1) / size_t_bits;
    
            std::size_t seed = 0;
            T positive = val < 0 ? -1 - val : val;

            for(unsigned int i = length * size_t_bits; i > 0; i -= size_t_bits)
            {
                seed ^= (std::size_t) (positive >> i) + (seed<<6) + (seed>>2);
            }
            seed ^= (std::size_t) val + (seed<<6) + (seed>>2);
            return seed;            
        }
        std::size_t seed_;
    };    

} // engine
