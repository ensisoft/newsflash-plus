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

namespace newsflash
{
    class buffer;

    // IO interface for consuming task IO specific buffers.
    class taskio
    {
    public:
        virtual ~taskio() = default;

        // prepare the IO object to receive buffers
        virtual void prepare() = 0;

        // receive a buffer full of NNTP data.
        virtual void receive(const buffer& buff) = 0;

        // cancel the task, roll back uncommited changes
        // and release held resources. no more buffers follow.
        virtual void cancel() = 0;

        // flush a temporary snapshot to the disk
        // and commit changes so far.
        virtual void flush() = 0;

        // finalize the task, commit final status of the task
        // to the disk and release held resources.
        // no more buffers follow.
        virtual void finalize() = 0;
    protected:
    private:
    };

} // newsflash
