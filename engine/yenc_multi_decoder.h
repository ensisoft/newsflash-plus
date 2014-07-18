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

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <boost/crc.hpp>
#include <newsflash/warnpop.h>
#include <string>
#include "decoder.h"

namespace newsflash
{
    // decoder for multi part yenc encoded content
    class yenc_multi_decoder : public decoder
    {
    public:
        using decoder::decode;

        yenc_multi_decoder();

       ~yenc_multi_decoder();

        // receive a part of the media in an yenc encoded buffer.
        virtual std::size_t decode(const void* data, std::size_t len) override;

        virtual void finish() override;
    private:
        boost::crc_32_type crc_;
        std::uint32_t crc_value_;
        std::size_t part_count_;
        std::size_t part_;
        std::size_t binary_size_;
        std::size_t size_;
        bool has_header_;
    };

} // newsflash
