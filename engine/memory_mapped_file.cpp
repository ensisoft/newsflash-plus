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

#if defined(WINDOWS_OS)
#  include <windows.h>
#elif defined(LINUX_OS)
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <sys/mman.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <cerrno>
#endif

#include <vector>
#include <algorithm>
#include <cassert>
#include "assert.h"
#include "bigfile.h"
#include "memory_mapped_file.h"
#include "utf8.h"

namespace {

struct chunk {
    char* data;
    std::size_t offset;
    std::size_t size;
    std::size_t ref;
};

} // namespace

namespace newsflash
{

#if defined(WINDOWS_OS)

struct memory_mapped_file::impl 
{
    HANDLE file;
    HANDLE map;

    impl() : file(NULL), map(NULL)
    {}

   ~impl()
    {
        if (map)
            CHECK(CloseHandle(map), TRUE);
        if (file)
            CHECK(CloseHandle(file), TRUE);
    }

    std::error_code map_file(const std::string& filename)
    {
        const std::wstring& wstr = utf8::decode(filename);

        const HANDLE fd = CreateFile(
            wstr.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if (fd == INVALID_HANDLE_VALUE)
            return std::error_code(GetLastError(), std::system_category());

        const HANDLE mp = CreateFileMapping(
            fd, 
            NULL, // default security
            PAGE_READWRITE,
            0, // high size word
            map_size, 
            NULL); 
        if (mp == NULL)
            throw std::runtime_error("file mapping failed");

        // we're possibly mapping large amounts of data into the process's
        // address space so we'll set the inherit handle off. So any processes
        // that this process creates do not inherit the handles.
        SetHandleInformation(fd, HANDLE_FLAG_INHERIT, 0);
        SetHandleInformation(mp, HANDLE_FLAG_INHERIT, 0);
        file = fd;
        map  = mp;
        return std::error_code();
    }

    chunk map(std::size_t offset, std::size_t size)
    {
        void* ptr = MapViewOfFile(map, 
            FILE_MAP_WRITE,
            0,
            offset,
            size);
        if (ptr == NULL)
            throw std::runtime_error("map failed");
    }

    void unmap(const chunk& c)
    {
        CHECK(UnmapViewOfFile(c.data), TRUE);
    }

};


std::size_t get_page_size()
{
    SYSTEMINFO sys {0};
    GetSystemInfo(&sys);
    return sys.dwAllocationGranularity;
}


#endif

#if defined(LINUX_OS)    

std::size_t get_page_size()
{
    return getpagesize();

}

struct memory_mapped_file::impl 
{
    int file;    

    impl() : file(0)
    {}

   ~impl()
    {
        if (file)
            CHECK(close(file), 0);
    }

    std::error_code map_file(const std::string& filename)
    {
        int fd = ::open(filename.c_str(), O_RDWR | O_LARGEFILE);
        if (fd == -1)
            return std::error_code(errno, std::generic_category());

        assert(!file);
        file = fd;
        return std::error_code();
    }

    void unmap(const chunk& c)
    {
        CHECK(munmap(c.data, c.size), 0);
    }
    chunk map(std::size_t offset, std::size_t size)
    {
        void* ptr = mmap(0, size, 
            PROT_READ | PROT_WRITE, // data can be read and written
            MAP_SHARED, /* MAP_POPULATE */ // changes are shared
            file,
            offset);
        if (ptr == MAP_FAILED)
            throw std::runtime_error("map failed");

        return chunk {(char*)ptr, offset, size, 0};
    }
};

#endif

memory_mapped_file::memory_mapped_file() : map_max_count_(0), map_size_(0), file_size_(0)
{}

memory_mapped_file::~memory_mapped_file()
{
    if (!pimpl_)
        return;

    for (auto& c : chunks_)
    {
        pimpl_->unmap(c);
    }
}


std::error_code memory_mapped_file::map(const std::string& file)
{
    // we create a single chunk mapping that has a chunksize covering
    // the whole file contents
    std::unique_ptr<impl> map(new impl);

    const auto ret = map->map_file(file);
    if (ret != std::error_code())
        return ret;

    pimpl_         = std::move(map);
    file_size_     = bigfile::size(file).second;    
    map_max_count_ = 1;
    map_size_      = file_size_;

    return ret;
}

std::error_code memory_mapped_file::map(const std::string& file, std::size_t map_size, std::size_t map_max_count)
{
    std::unique_ptr<impl> map(new impl);

    const auto ret = map->map_file(file);

    if (ret != std::error_code())
        return ret;

    const auto pagesize = get_page_size();

    pimpl_         = std::move(map);
    file_size_     = bigfile::size(file).second;    
    map_max_count_ = map_max_count;
    map_size_      = (map_size + pagesize - 1) & ~(pagesize - 1);

    return ret;
}

void* memory_mapped_file::data(std::size_t offset, std::size_t size)
{
    assert(size <= map_size_);

    const auto chunk_offset = offset % map_size_;

    // see if we can find a block that can accomodate the 
    // requested offset and size
    const auto it = std::find_if(chunks_.begin(), chunks_.end(),
        [=](const chunk& c)
        {
            if (c.offset <= offset)
            {
                if (c.size - chunk_offset >= size)
                    return true;
            }

            return false;
        });

    chunk* ret = nullptr;

    if (it != chunks_.end())
    {
        ret = &*it;
    }
    else
    {
        // no chunk was found. map a new chunk, but before that
        // check if need to evict some chunk. if so, evict the 
        // chunk with smallest ref count.
        if (chunks_.size() == map_max_count_)
        {
            auto it = std::min_element(chunks_.begin(), chunks_.end(), 
                [](const chunk& lhs, const chunk& rhs)
                {
                    return lhs.ref < rhs.ref;
                });

            auto& chunk = *it;
            pimpl_->unmap(chunk);

            *it = chunks_.back();
            chunks_.pop_back();
        }
        chunks_.reserve(chunks_.size() + 1);

        const auto chunk_index  = offset / map_size_;
        const auto chunk_offset = chunk_index * map_size_;

        chunk c = pimpl_->map(chunk_offset, map_size_);
        chunks_.push_back(c);
        ret = &chunks_.back();
    }

    ret->ref++;
    return ret->data + chunk_offset;
}

std::size_t memory_mapped_file::file_size() const
{
    return file_size_;
}

std::size_t memory_mapped_file::map_count() const
{
    return chunks_.size();
}


} // newsflash