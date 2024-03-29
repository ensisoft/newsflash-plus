// Copyright (c) 2010-2015 Sami Väisänen, Ensisoft
//
// http://www.ensisoft.com
//
// This software is copyrighted software. Unauthorized hacking, cracking, distribution
// and general assing around is prohibited.
// Redistribution and use in source and binary forms, with or without modification,
// without permission are prohibited.
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

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
#include <stdexcept>
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
    mapper(const std::string& filename)
    {
        const auto str = utf8::decode(filename);
        file_ = CreateFile(
            str.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if (file_ == INVALID_HANDLE_VALUE)
            throw std::system_error(std::error_code(GetLastError(), std::system_category()),
                "filemap open failed: " + filename);

        LARGE_INTEGER size;
        if (!GetFileSizeEx(file_, &size)) {
            CloseHandle(file_);
            throw std::runtime_error("get file size failed");
        }
        size_ = size.QuadPart;

        mmap_ = CreateFileMapping(
            file_,
            NULL, // default security
            PAGE_READWRITE,
            size.HighPart,
            size.LowPart,
            NULL);
        if (mmap_ == NULL)
        {
            CloseHandle(file_);
            throw std::runtime_error("file mapping failed");
        }

        // we're possibly mapping large amounts of data into the process's
        // address space so we'll set the inherit handle off. So any processes
        // that this process creates do not inherit the handles.
        SetHandleInformation(file_, HANDLE_FLAG_INHERIT, 0);
        SetHandleInformation(mmap_, HANDLE_FLAG_INHERIT, 0);

        base_ = MapViewOfFile(mmap_,
            FILE_MAP_READ | FILE_MAP_WRITE,
            0, 0, size_);
        if (base_ == NULL)
        {
            CloseHandle(file_);
            throw std::runtime_error("MapViewOfFile failed");
        }
    }

   ~mapper()
    {
        ASSERT(UnmapViewOfFile(base_) == TRUE);
        ASSERT(CloseHandle(mmap_) == TRUE);
        ASSERT(CloseHandle(file_) == TRUE);
    }

    void* map(std::size_t offset /*, std::size_t size,  unsigned flags*/)
    {
        return (char*)base_ + offset;
    }

    void flush()
    {
        if (FlushViewOfFile(base_, 0) == 0)
            throw std::runtime_error("flush memory map failed");
    }

    std::size_t size() const
    {
        return size_;
    }
private:
    HANDLE file_ = INVALID_HANDLE_VALUE;
    HANDLE mmap_ = INVALID_HANDLE_VALUE;
    void* base_  = nullptr;
    std::size_t size_ = 0;
};

#endif

#if defined(LINUX_OS)

class filemap::mapper
{
public:
    mapper(const std::string& filename) : file_(0)
    {
        file_ = ::open(filename.c_str(), O_RDWR | O_LARGEFILE);
        if (file_ == -1)
            throw std::system_error(std::error_code(errno, std::generic_category()),
                "filemap open failed: " + filename);

        struct stat64 st;
        if (fstat64(file_, &st)) {
            ::close(file_);
            throw std::runtime_error("filestat failed");
        }
        size_ = st.st_size;
        base_ = ::mmap(0, size_, PROT_READ | PROT_WRITE, MAP_SHARED, file_, 0);
        if (base_ == MAP_FAILED) {
            ::close(file_);
            throw std::runtime_error("mmap failed");
        }
    }

   ~mapper()
    {
        ASSERT(::munmap(base_, size_) == 0);
        ASSERT(::close(file_) == 0);
    }
    void* map(std::size_t offset /*, std::size_t size, unsigned flags*/)
    {
        return (char*)base_ + offset;
    }

    void flush()
    {
        if (::msync(base_, size_, MS_INVALIDATE) == -1)
            throw std::runtime_error("flush memory map failed");
    }

    std::size_t size() const
    {
        return size_;
    }

private:
    int file_ = -1;
    void* base_ = nullptr;
    std::size_t size_ = 0;
};

#endif

filemap::buffer::~buffer()
{}

void filemap::open(const std::string& file)
{
    auto map = std::make_shared<mapper>(file);

    mapper_   = map;
    filename_ = std::move(file);
}

void filemap::close()
{
    mapper_.reset();
    filename_.clear();
}

void filemap::flush()
{
    mapper_->flush();
}

void* filemap::map_ptr(std::size_t offset, std::size_t size)
{
    const auto end = offset + size;
    if (end > mapper_->size())
        throw std::runtime_error("filemap offset past the end of file");

    return mapper_->map(offset);
}

filemap::buffer filemap::load(std::size_t offset, std::size_t size, unsigned flags)
{
    const auto end = offset + size;
    if (end > mapper_->size())
        throw std::runtime_error("filemap offset past the end of file");

    auto* p = (char*)mapper_->map(offset);//, len, flags);

    return {mapper_, size, p};
}

std::size_t filemap::size() const
{
    return mapper_->size();
}

std::string filemap::filename() const
{
    return filename_;
}

bool filemap::is_open() const
{
    return !!mapper_;
}

} // newsflash

