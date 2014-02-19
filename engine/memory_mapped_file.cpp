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

#include <newsflash/config.h>

#if defined(LINUX_OS)
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <sys/mman.h>
#  include <unistd.h>
#  include <cerrno>
#endif

#include <vector>
#include "assert.h"
#include "memory_mapped_file.h"

namespace {

struct mem_map {
    void* data;
    std::size_t offset;
    std::size_t size;
    std::size_t ref;
};

} // namespace

namespace newsflash
{

#if defined(WINDOWS_OS)

struct memory_mapped_file::impl {
    const std::string filename;
    const std::size_t map_max_count;
    const std::size_t map_size;
    HANDLE file;
    HANDLE map;


};

std::size_t get_page_size()
{

}


#endif

#if defined(LINUX_OS)    

std::size_t get_page_size()
{
    return ::getpagesize();

}

struct memory_mapped_file::impl {
    const std::string filename;
    const std::size_t map_max_count;
    const std::size_t map_size;
    int file;    

    impl(std::string file, std::size_t map_count, std::size_t map_size) 
        : filename(std::move(file)), map_max_count(map_count), map_size(map_size), file(0)
    {}

    std::error_code map()
    {
        return std::error_code();
    }
};

#endif

memory_mapped_file::memory_mapped_file()
{}

memory_mapped_file::~memory_mapped_file()
{}


std::error_code memory_mapped_file::map(const std::string& file)
{
    std::unique_ptr<impl> map(new impl(file, 0, 0));

    const auto ret = map->map();
    if (ret == std::error_code())
        pimpl_ = std::move(map);

    return ret;
}

std::error_code memory_mapped_file::map(const std::string& file, std::size_t map_size, std::size_t map_max_count)
{
    const auto pagesize = get_page_size();
    map_size = (map_size + pagesize - 1) & ~(pagesize - 1);

    std::unique_ptr<impl> map(new impl(file, map_max_count, map_size));

    const auto ret = map->map();
    if (ret == std::error_code())
        pimpl_ = std::move(map);

    return ret;
}


} // newsflash