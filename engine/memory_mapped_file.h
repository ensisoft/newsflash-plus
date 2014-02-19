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
#include <system_error>

namespace newsflash
{
    class memory_mapped_file
    {
    public:
        memory_mapped_file();

       ~memory_mapped_file();

        // map the given file fully into process.
        std::error_code map(const std::string& file);

        // map the given file into memory in at least map_size chunks 
        // at any given time a maximum of map_max_count of chunks are resident 
        // in memory. unmapping of chunks is done transparently 
        // when new data pointers are requested.
        std::error_code map(const std::string& file, std::size_t map_size, std::size_t map_max_count);

        // retrive pointer to the data at the specified offset.
        // call to this function may invalidate any previously
        // retrieved data pointer if a transparent unmap is done.
        void* data(std::size_t offset);

    private:
        struct impl;

        std::unique_ptr<impl> pimpl_;
    };

} // newsflash
