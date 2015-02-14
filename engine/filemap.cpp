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

class filemap::mapper
{
public:
    mapper(const std::string& filename) : file_(NULL), mmap_(NULL)
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

        const auto map = CreateFileMapping(
            fd, 
            NULL, // default security
            PAGE_READWRITE,
            0, // high size word
            0, // low size word  (when the mapping size is 0 the current maximum size of the file mapping equals the file size)
            NULL); 
        if (map == NULL)
        {
            CloseHandle(fd);
            throw std::runtime_error("file mapping failed");
        }

        // we're possibly mapping large amounts of data into the process's
        // address space so we'll set the inherit handle off. So any processes
        // that this process creates do not inherit the handles.
        SetHandleInformation(fd, HANDLE_FLAG_INHERIT, 0);
        SetHandleInformation(map, HANDLE_FLAG_INHERIT, 0);
        file_ = fd;
        mmap_ = map;
    }

   ~mapper()
    {
        ASSERT(CloseHandle(mmap_) == TRUE);
        ASSERT(CloseHandle(file_) == TRUE);
    }    

    void* map(std::size_t offset, std::size_t size,  unsigned flags)
    {
        int read_write_flags = FILE_MAP_READ;
        if (flags & filemap::buf_write)
            read_write_flags = FILE_MAP_WRITE;

        void* ptr = MapViewOfFile(mmap_, 
            read_write_flags,
            0,
            offset,
            size);
        if (ptr == NULL)
            throw std::runtime_error("map failed");

        return ptr;
    }

    void unmap(void* base, std::size_t)
    {
        ASSERT(UnmapViewOfFile(base) == TRUE);
    }

    std::size_t size() const 
    {
        LARGE_INTEGER size;
        if (!GetFileSizeEx(file_, &size))
            throw std::runtime_error("get file size failed");
        return size.QuadPart;
    }
private:
    HANDLE file_;
    HANDLE mmap_;    
};


std::size_t get_page_size()
{
    SYSTEM_INFO sys {0};
    GetSystemInfo(&sys);
    return sys.dwAllocationGranularity;
}

#endif

#if defined(LINUX_OS)    

class filemap::mapper
{
public:
    mapper(const std::string& filename) : file_(0)
    {
        int fd = ::open(filename.c_str(), O_RDWR | O_LARGEFILE);
        if (fd == -1)
            throw std::system_error(std::error_code(errno, std::generic_category()),
                "filemap open failed: " + filename);
        file_ = fd;
    }

   ~mapper()
    {
        ASSERT(close(file_) == 0);
    }
    void unmap(void* ptr, std::size_t size)
    {
        ASSERT(munmap(ptr, size) == 0);
    }
    void* map(std::size_t offset, std::size_t size, unsigned flags)
    {
        int read_write_flags = 0;
        if (flags & filemap::buf_read)
            read_write_flags |= PROT_READ;
        if (flags & filemap::buf_write)
            read_write_flags |= PROT_WRITE;

        int share_flags = MAP_SHARED;

        void* ptr = ::mmap(0, size, 
            read_write_flags,
            share_flags,
            file_,
            offset);
        if (ptr == MAP_FAILED)
            throw std::runtime_error("map failed");

        return ptr;
    }
    std::size_t size() const 
    {
        struct stat64 st;
        if (fstat64(file_, &st))
            throw std::runtime_error("filestat failed");
        return st.st_size;
    }

private:
    int file_;        
};

std::size_t get_page_size()
{
    return getpagesize();
}

#endif

filemap::buffer::~buffer()
{
    if (base_) // could have been moved
    {
        mapper_->unmap(base_, length_);
    }
}

filemap::filemap()
{}

filemap::~filemap()
{}

void filemap::open(std::string file)
{
    auto map = std::make_shared<mapper>(file);

    mapper_   = map;
    filename_ = std::move(file);
}


filemap::buffer filemap::load(std::size_t offset, std::size_t size, unsigned flags)
{
    static const auto pagesize = get_page_size();
    const auto beg = (offset / pagesize) * pagesize;
    const auto len = size + (offset % pagesize);
    const auto end = offset + size;

    auto* p = (char*)mapper_->map(beg, len, flags);

    return {mapper_, size, (offset % pagesize), p};
}

std::size_t filemap::size() const
{
    return mapper_->size();
}

} // newsflash

