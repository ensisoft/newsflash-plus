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

#include <memory>
#include "buffer.h"
#include "protocol.h"
#include "cmdlist.h"

namespace newsflash
{
    // generate request to download group listing 
    class grouplist : public cmdlist
    {
    public:
        struct list {
        };

        virtual bool run(protocol& proto) override
        {
            if (!first_thread())
                return false;

            auto buff = std::make_shared<buffer>(1024*1024);
            proto.list(*buff);
            return false;
        }

    private:
        bool first_thread()
        {
            std::unique_lock<std::mutex> lock(mutex_);
            bool ret = first_;
            first_ = false;
            return ret;
        }

    private:
        std::mutex mutex_;
        bool first_;
    };

} // newsflash
