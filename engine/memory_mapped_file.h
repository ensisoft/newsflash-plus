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

#include <system_error>
#include <memory>
#include <vector>

namespace newsflash
{
    class memory_mapped_file
    {
    public:
        memory_mapped_file();

       ~memory_mapped_file();

        // map the given file fully into process and let the implementation
        // figure out optimal values for map_size and map_max_count
        std::error_code map(const std::string& file);

        // map the given file into memory in at least map_size chunks 
        // at any given time a maximum of map_max_count of chunks are resident 
        // in memory. unmapping of chunks is done transparently 
        // when new data pointers are requested.
        // if map_size is not a multiple of a system page size it's aligned
        // upwards to a whole page multiple.
        std::error_code map(const std::string& file, std::size_t map_size, std::size_t map_max_count);

        // retrive pointer to the data at the specified offset.
        // call to this function may invalidate any previously
        // retrieved data pointer if a transparent unmap is done.
        // size should be less than or equal to map_size
        void* data(std::size_t offset, std::size_t size);

        // get file size
        std::size_t file_size() const;

        // get current map count. the maximum memory consumption at any time is
        // map_size (aligned to multiple of system page size) * map_count() + map_size
        // the addition of the extra map_size is for cases where a request for a block
        // of data falls across map chunk boundries, in which case the mapped chunk
        // is increased in size to provide contiguous access to the data.
        std::size_t map_count() const;

    private:
        struct chunk {
            char* data;
            std::size_t offset;
            std::size_t size;
            std::size_t ref;
        };

        chunk* find_chunk(std::size_t offset, std::size_t size);
        chunk* map_chunk(std::size_t offset, std::size_t size);

    private:
        void evict();

    private:
        struct impl;

        std::unique_ptr<impl> pimpl_;

        std::vector<chunk> chunks_;
        std::size_t map_max_count_;
        std::size_t map_size_;
        std::size_t map_file_size_;
    };

} // newsflash
