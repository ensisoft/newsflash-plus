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
#include <memory>
#include <map>

namespace newsflash
{
    // filemap maps the contents of a file on the disk into system ram in variable size chunks. 
    class filemap
    {
    public:
        filemap();
       ~filemap();

        void open(std::string file, bool create_if_not_exists);

        // retrive pointer to the data at the specified offset.
        void* mem_map(std::size_t offset, std::size_t size);

        void  mem_unmap(void* mem, std::size_t size);

    private:
        struct impl;

        std::unique_ptr<impl> pimpl_;
        std::string filename_;
    #ifdef NEWSFLASH_DEBUG
        std::map<void*, std::size_t> maps_;
    #endif
    };

} // newsflash
