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

#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <memory>
#include <cstdint>
#include "cmdlist.h"

namespace newsflash
{
    class buffer;

    // generate a list of xover commands to retrive
    // the headers of a group
    class xoverlist : public cmdlist
    {
    public:
        struct xover {
            std::shared_ptr<buffer> buff;
            std::uint64_t start;
            std::uint64_t end;
        };

        // callback to be invoked on xover data
        std::function<void (const xoverlist::xover& xover)> on_xover;

        //std::functio

        xoverlist(std::string group);

        virtual bool run(protocol& proto) override;

    private:
        bool first_thread();

    private:
        const std::string group_;

    private:
        std::condition_variable cond_;
        std::mutex mutex_;
        std::uint64_t start_;
        std::uint64_t end_;
        bool first_;
        bool error_;
        bool configured_;
    };

} // newsflash
