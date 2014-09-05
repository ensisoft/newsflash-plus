// Copyright (c) 2013 Sami Väisänen, Ensisoft 
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

#include <type_traits>

namespace newsflash
{
    template<typename Enum>
    class bitflag
    {
    public:
        using Bits = typename std::underlying_type<Enum>::type;

        bitflag() : bits_(0)
        {}
        bitflag(Enum initial) : bits_(0)
        {
            set(initial);
        }

        bitflag& set(Enum value) 
        {
            bits_ |= (1 << Bits(value));
            return *this;
        }
        bitflag& unset(Enum value)
        {
            bits_ &= ~(1 << value);
            return *this;
        }

        bitflag& operator |= (bitflag other)
        {
            bits_ |= other.bits_;
            return *this;
        }

        bitflag& operator &= (bitflag other)
        { 
            bits_ &= other.bits_;
            return *this;
        }

        bool test(Enum value) const
        { return bits_ & (1 << Bits(value)); }

        void clear() 
        { bits_ = 0x0; }

        bool any_bit() const 
        { return bits_ != 0; }

        Bits value() const 
        { return bits_; }        

    private:
        bitflag(Bits b) : bits_(b)
        {}
    private:
        template<typename E> friend bitflag operator | (bitflag<E>, bitflag<E>);
        template<typename E> friend bitflag operator & (bitflag<E>, bitflag<E>);

    private:
        Bits bits_;
    };

    template<typename Enum>
    auto operator | (bitflag<Enum> lhs, bitflag<Enum> rhs) -> decltype(lhs)
    {
        return { lhs.bits_ | rhs.bits_ };
    }

    template<typename Enum>
    auto operator & (bitflag<Enum> lhs, bitflag<Enum> rhs) -> decltype(lhs)
    {
        return { lhs.bits_ & rhs.bits_ };
    }

} // newsflash


