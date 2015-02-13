// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
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
#  include "utf8.h"
#elif defined(LINUX_OS)
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <cerrno>
#endif
#include <system_error>
#include "filebuf.h"
#include "assert.h"

namespace newsflash
{

#if defined(WINDOWS_OS)

class filebuf::fileio
{
public:
    fileio(const std::string& file)
    {
        const auto str = utf8::decode(file);
        const auto fd  = CreateFile(
            str.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if (fd == INVALID_HANDLE_VALUE)
            throw system_error(std::error_code(GetLastError(), std::system_category()),
                "filebuf open failed: " + file);
        file_ = fd;
    }
   ~fileio()
    {
        ASSERT(CloseHandle(file_) == TRUE);
    }

    void read(void* buff, std::size_t bytes, std::size_t offset)
    {}

    void write(const void* buff, std::size_t bytes, std::size_t offset)
    {}

    std::size_t size() const 
    {
        LARGE_INTEGER size;
        if (!GetFileSizeEx(file_, &size))
            throw std::runtime_error("get file size failed");

        return size.QuadPart;
    }

private:
    HANDLE file_;
};

#endif

#if defined(LINUX_OS)

class filebuf::fileio
{
public:
    fileio(const std::string& file)
    {
        int fd = ::open(file.c_str(), O_RDWR | O_LARGEFILE | O_CREAT, 
            S_IRWXU | S_IRGRP | S_IROTH);
        if (fd == -1)
            throw std::system_error(std::error_code(errno,
                std::generic_category()), "filebuf open failed: " + file);
        file_ = fd;
    }
   ~fileio()
    {
        ASSERT(::close(file_) == 0);
    }
    void read(void* buff, std::size_t bytes, std::size_t offset)
    {
        if (::lseek64(file_, offset, SEEK_SET) == off64_t(-1))
            throw std::runtime_error("file seek failed");
        if (::read(file_, buff, bytes) == -1)
            throw std::runtime_error("file read failed");
    }

    void write(const void* buff, std::size_t bytes, std::size_t offset)
    {
        // lseek allows the seek to be set beyond the existing end of file.
        if (::lseek64(file_, offset, SEEK_SET) == off64_t(-1))
            throw std::runtime_error("file seek failed");
        if (::write(file_, buff, bytes) == -1)
            throw std::runtime_error("file write failed");

        //::fdatasync(file_); // todo: ??
    }

    std::size_t size() const
    {
        struct stat64 st;
        if (fstat64(file_, &st))
            throw std::runtime_error("failed to get file size");
        return st.st_size;
    }
private:
    int file_;
};

#endif

void filebuf::buffer::flush()
{
    if (!write_) return;

    fileio_->write(&data_[0], data_.size(), offset_);
}

void filebuf::open(std::string file)
{
    auto io = std::make_shared<fileio>(file);

    fileio_ = io;
    filename_ = std::move(file);
}

filebuf::buffer filebuf::load(std::size_t offset, std::size_t size, unsigned flags)
{
    std::vector<byte> vec;
    vec.resize(size);

    const bool read_data  = flags & buf_read;
    const bool write_data = flags & buf_write;
    if (read_data)
        fileio_->read(&vec[0], size, offset);

    return {fileio_, std::move(vec), offset, write_data};
}

std::size_t filebuf::size() const 
{
    return fileio_->size();
}

} // newsflash