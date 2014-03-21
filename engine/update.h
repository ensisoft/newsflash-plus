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

#include <set>
#include <memory>
#include <cstdint>
#include "task.h"

namespace corelib
{
    // class update : public task
    // {
    // public:
    //     enum class flags {
    //         merge_old       = 0x1,
    //         mark_old_new    = 0x2,
    //         index_by_author = 0x4,
    //         index_by_date   = 0x8,
    //         ids64           = 0x10,
    //         crc16           = 0x20
    //     };


    //     virtual void prepare() override;
    //     virtual void receive(std::shared_ptr<const buffer> buff, std::size_t id) override;
    //     virtual void cancel() override;
    //     virtual void flush() override;
    //     virtual void finalize() override;

    // private:


    // private:

    // };

} // corelib
