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
#include <atomic>

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

        virtual ~cmdlist() = default;

        // try the ith step to configure the session state
        virtual bool submit_configure_command(std::size_t i, session& ses) = 0;

        // receive and handle ith response to ith configure command.
        virtual bool receive_configure_buffer(std::size_t i, buffer&& buff) = 0;

        // submit the data transfer commands as a single batch.
        virtual void submit_data_commands(session& ses) = 0;

        // receive a data buffer response to submit_data_commands
        virtual void receive_data_buffer(buffer&& buff) = 0;

        virtual void cancel()
        { cancel_ = true; }
        
        virtual bool is_canceled() const
        { return cancel_;}

    protected:
        std::atomic<bool> cancel_;
    private:
    };

} // newsflash
