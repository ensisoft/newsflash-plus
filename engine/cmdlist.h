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

#include <cstddef>

namespace newsflash
{
    class buffer;
    class session;

    class cmdlist
    {
    public:
        enum class step {
            configure, transfer
        };

        virtual ~cmdlist() = default;

        virtual bool is_done(cmdlist::step step) const = 0;

        virtual bool is_good(cmdlist::step step) const = 0;

        virtual void submit(cmdlist::step step, session& ses) = 0;

        virtual void receive(cmdlist::step step, buffer& buff) = 0;

        virtual void next(cmdlist::step step) = 0;

        virtual std::size_t account() const = 0;

        virtual std::size_t task() const = 0;
    protected:
    private:
    };

} // newsflash
