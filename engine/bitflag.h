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

        enum {
            BitCount = sizeof(Bits) * 8
        };

        bitflag() : bits_(0)
        {}
        bitflag(Enum initial) : bits_(0)
        {
            set(initial);
        }
        bitflag(Bits value) : bits_(value)
        {}

        bitflag& set(Enum value, bool on = true) 
        {
            if (on)
                bits_ |= (1 << Bits(value));
            else bits_ &= ~(1 << Bits(value));
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

        // test a particular value.
        bool test(Enum value) const
        { 
            const auto b = 1 << Bits(value);
            return (bits_ & b) == b;
        }
        
        // test for any value.
        bool test(bitflag values) const 
        { return bits_ & values.bits_; }

        // test of the nth bith.
        bool test(unsigned index) const 
        {
            const auto b = 1 << index;
            return (bits_ & b);
        }

        void clear() 
        { bits_ = 0x0; }

        bool any_bit() const 
        { return bits_ != 0; }

        Bits value() const 
        { return bits_; }        

        void set_from_value(Bits b)
        { bits_ = b; }
    private:
        Bits bits_;
    };

    // we only provide this operator, since its global
    // this also covers Enum | bitflag<Enum> and bitflag<Enum> | Enum
    // through implicit conversion to bitflag<Enum>
    template<typename Enum>
    auto operator | (bitflag<Enum> lhs, bitflag<Enum> rhs) -> decltype(lhs)
    {
        return { lhs.value() | rhs.value() };
    }

    template<typename Enum>
    auto operator | (bitflag<Enum> lhs, Enum e) -> decltype(lhs)
    {
        return lhs | bitflag<Enum>(e);
    }

    template<typename Enum>
    auto operator & (bitflag<Enum> lhs, bitflag<Enum> rhs) -> decltype(lhs)
    {
        return bitflag<Enum>(lhs.value() & rhs.value());
    }

    template<typename Enum>
    auto operator & (bitflag<Enum> lhs, Enum e) -> decltype(lhs)
    {
        return lhs & bitflag<Enum>(e);
    }

} // newsflash


