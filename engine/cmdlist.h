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

    // cmdlist encapsulates a sequence of nntp opereations to be performed
    // for example downloading articles or header overview data.
    class cmdlist
    {
    public:
        // the cmdlist operation is divided into 2 steps, session configuration
        // and then the actual data transfer. 
        // session configuration is performed step-by-step (without pipelining)
        // where as the data transfer can be pipelined. 
        enum class step {
            configure, transfer
        };

        virtual ~cmdlist() = default;

        // check if the step is done.
        // returns true if done, otherwise false and next operation is performed.
        virtual bool is_done(cmdlist::step step) const = 0;

        // check if the step was completed succesfully. if step is 
        // configure and the return value is false then transfer is not performed.
        virtual bool is_good(cmdlist::step step) const = 0;

        // perform the next operation in the session for the given step.
        virtual void submit(cmdlist::step step, session& ses) = 0;

        // receive next response buffer for the given step.
        virtual void receive(cmdlist::step step, buffer&& buff) = 0;

        // move onto next command for the given step.
        virtual void next(cmdlist::step step) = 0;

        // get the account to which this cmdlist is associated with.
        virtual std::size_t account() const = 0;

        // get the owner task id. 
        virtual std::size_t task() const = 0;
        
    protected:
    private:
    };

} // newsflash
