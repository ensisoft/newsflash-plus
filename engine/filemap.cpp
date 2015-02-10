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

#include <system_error>
#include <vector>
#include <algorithm>
#include <cassert>
#include "assert.h"
#include "bigfile.h"
#include "filemap.h"
#include "utf8.h"

namespace newsflash
{

#if defined(WINDOWS_OS)

struct filemap::impl 
{
    HANDLE file;
    HANDLE map;

    impl(const std::string& filename) : file(NULL), map(NULL)
    {
        const auto str = utf8::decode(filename);
        const auto fd  = CreateFile(
            str.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if (fd == INVALID_HANDLE_VALUE)
            throw std::system_error(std::error_code(GetLastError(), std::system_category()),
                "filemap open failed: " + filename);

        const auto mp = CreateFileMapping(
            fd, 
            NULL, // default security
            PAGE_READWRITE,
            0, // high size word
            map_size, 
            NULL); 
        if (mp == NULL)
        {
            CloseHandle(fd);
            throw std::runtime_error("file mapping failed");
        }

        // we're possibly mapping large amounts of data into the process's
        // address space so we'll set the inherit handle off. So any processes
        // that this process creates do not inherit the handles.
        SetHandleInformation(fd, HANDLE_FLAG_INHERIT, 0);
        SetHandleInformation(mp, HANDLE_FLAG_INHERIT, 0);
        file = fd;
        map  = mp;
    }

   ~impl()
    {
        ASSERT(CloseHandle(map) == TRUE);
        ASSERT(CloseHandle(file) == TRUE);
    }    

    void* map(std::size_t offset, std::size_t size)
    {
        void* ptr = MapViewOfFile(map, 
            FILE_MAP_WRITE,
            0,
            offset,
            size);
        if (ptr == NULL)
            throw std::runtime_error("map failed");

        return ptr;
    }

    void unmap(const chunk& c)
    {
        ASSERT(UnmapViewOfFile(c.data) == TRUE);
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

struct filemap::impl 
{
    int file;    

    impl(const std::string& filename, bool create) : file(0)
    {
        int mode = O_RDWR | O_LARGEFILE;
        if (create)
            mode |= O_CREAT;
        int fd = ::open(filename.c_str(), mode);
        if (fd == -1)
            throw std::system_error(std::error_code(errno, std::generic_category()),
                "filemap open failed: " + filename);
        file = fd;
    }

   ~impl()
    {
        ASSERT(close(file) == 0);
    }

    void unmap(void* ptr, std::size_t size)
    {
        ASSERT(munmap(ptr, size) == 0);
    }
    void* map(std::size_t offset, std::size_t size)
    {
        void* ptr = mmap(0, size, 
            PROT_READ | PROT_WRITE, // data can be read and written
            MAP_SHARED, /* MAP_POPULATE */ // changes are shared
            file,
            offset);
        if (ptr == MAP_FAILED)
            throw std::runtime_error("map failed");

        return ptr;
    }
};

#endif

std::size_t align(std::size_t s, std::size_t boundary)
{
    return (s + boundary - 1) & ~(boundary - 1);
}

filemap::filemap() 
{}


filemap::~filemap()
{
    if (!pimpl_)
        return;

#ifdef NEWSFLASH_DEBUG
    assert(maps_.size());
#endif
}


void filemap::open(std::string file, bool create_if_not_exists)
{
    std::unique_ptr<impl> map(new impl(file, create_if_not_exists));

#ifdef NEWSFLASH_DEBUG
    assert(maps_.empty());
#endif

    pimpl_ = std::move(map);
}


void* filemap::mem_map(std::size_t offset, std::size_t size)
{
    const auto pagesize = get_page_size();
    const auto index = offset / pagesize;
    const auto start = index * pagesize;
    const auto bytes = align(size, pagesize);
    const auto end   = start + bytes;

    const auto ret = bigfile::size(filename_);
    if (ret.first)
        throw std::system_error(ret.first, "get filesize failed");

    const auto cur_file_size = ret.second;

    if (end > cur_file_size)
        bigfile::resize(filename_, end);

    void* p = pimpl_->map(start, end);

#ifdef NEWSFLASH_DEBUG
    maps_[p] = size;
#endif

    return p;
}

void filemap::mem_unmap(void* mem, std::size_t size)
{
#ifdef NEWSFLASH_DEBUG
    auto it = maps_.find(mem);
    assert(it != std::end(maps_));
    maps_.erase(it);
#endif

    pimpl_->unmap(mem,size);
}

} // newsflash

