// Copyright (c) 2013,2014 Sami Väisänen, Ensisoft 
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

#include <memory>
#include <string>
#include <cstdint>
#include "msgqueue.h"

namespace newsflash
{
    struct command {
        size_t id;
        size_t taskid;
        size_t size; // expected buffer size        
    };

    class buffer;

    struct cmd_body : public command {
        enum class cmdstatus {
            success, unavailable, dmca
        };
        cmdstatus status;
        std::string groups[3];
        std::string article;
        std::shared_ptr<buffer> data;

        enum : size_t {
            ID = 1
        };
    };

    struct cmd_group : public command {
        std::string name;
        std::uint64_t high_water_mark;
        std::uint64_t low_water_mark;
        std::uint64_t article_count;
        bool success;

        enum : size_t {
            ID = 2
        };
    };

    struct cmd_xover : public command {
        std::string group;
        std::size_t start;
        std::size_t end;
        std::shared_ptr<buffer> data;
        bool success;

        enum : size_t {
            ID = 3
        };
    };

    struct cmd_list : public command {
        std::shared_ptr<buffer> data;

        enum : size_t {
            ID = 4
        };
    };

    typedef msgqueue cmdqueue;
    typedef msgqueue resqueue;

} // newsflash
