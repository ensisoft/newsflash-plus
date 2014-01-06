// Copyright (c) 2010-2013 Sami Väisänen, Ensisoft 
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

// $Id: file.cpp,v 1.12 2010/02/25 13:12:40 svaisane Exp $

#include <newsflash/config.h>
#if defined(WINDOWS_OS)
#  include <windows.h>
#  include <iterator>
#  include "utf8.h"
#elif defined(LINUX_OS)
#  ifndef __LARGEFILE64_SOURCE
#    define __LARGEFILE64_SOURCE
#  endif
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <cerrno>
#endif
#include <cassert>
#include <stdexcept>
#include "bigfile.h"
#include "assert.h"

namespace newsflash
{

#if defined(WINDOWS_OS)

struct bigfile::impl {
    HANDLE file;
    bool append;

    native_errcode_t open_file(const std::string& filename, unsigned flags)
    {
        assert(!filename.empty());

        const std::wstring& wide = utf8_decode(filename);

        const HANDLE file = CreateFile(
            wide.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            flags,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if (file == INVALID_HANDLE_VALUE)
            return GetLastError();

        // make the handle non-inheritable so that any child processes 
        // that get started by this process do not have these files open
        // todo: this is not really consistent behaviour with linux, should the 
        // handles be inherited?
        if (!SetHandleInformation(file, HANDLE_FLAG_INHERIT, 0))
        {
            const int err = GetLastError();
            CloseHandle(file);
            return err;
        }
        if (this->file)
            CHECK(CloseHandle(this->file), TRUE);

        this->file = file;
        return 0;
    }
};

bigfile::bigfile() : pimpl_(new impl)
{
    pimpl_->file = NULL;
    pimpl_->append = false;
}

native_errcode_t bigfile::open(const std::string& file)
{
    return pimpl_->open_file(file, OPEN_EXISTING);
}

native_errcode_t bigfile::append(const std::string& file)
{
    const native_errcode_t ret = pimpl_->open_file(file, OPEN_ALWAYS);
    if (ret == 0)
        pimpl_->append = true;

    return ret;
}

native_errcode_t bigfile::create(const std::string& file)
{
    return pimpl_->open_file(file, CREATE_ALWAYS);
}

bool bigfile::is_open() const
{
    return pimpl_->file != NULL;
}

void bigfile::close()
{
    if (pimpl_->file == NULL)
        return;

    CHECK(CloseHandle(pimpl_->file), TRUE);

    pimpl_->file   = NULL;
    pimpl_->append = false;
}

bigfile::big_t bigfile::position() const
{
    assert(is_open());

    LONG high = 0;
    DWORD low = SetFilePointer(pimpl_->file, 0, &high, FILE_CURRENT);
    if (low == 0xFFFFFFFF)
    {
        if (GetLastError() != NO_ERROR)
            throw std::runtime_error("get current file position failed");
    }

    const big_t ret = (big_t(high) << 32) | low;

    return ret;
}

void bigfile::seek(big_t offset)
{
    assert(is_open());
    assert(offset >= 0);

    LONG high = static_cast<LONG>(offset >> 32);
    DWORD low = SetFilePointer(pimpl_->file, static_cast<DWORD>(offset), &high, FILE_BEGIN);
    if (low == 0xFFFFFFFF)
    {
        if (GetLastError() != NO_ERROR)
            throw std::runtime_error("file seek failed");
    }
}

void bigfile::write(const void* data, size_t bytes)
{
    assert(is_open());
    assert(data);
    assert(bytes > 0 && bytes < ((size_t)-1));

    if (pimpl_->append)
    {
        if (!SetFilePointer(pimpl_->file, 0, NULL, FILE_END))
            throw std::runtime_error("file write failed (seek to the end)");
    }
    DWORD dw = 0;
    if (WriteFile(pimpl_->file, data, bytes, &dw, NULL) == 0)
        throw std::runtime_error("file write failed");
}

size_t bigfile::read(void* buff, size_t bytes)
{
    assert(is_open());
    assert(buff);
    assert(buff > 0 && bytes < ((size_t)-1));

    DWORD dw = 0;
    if (ReadFile(pimpl_->file, buff, bytes, &dw, NULL) == 0)
        throw std::runtime_error("file read failed");

    return size_t(dw);
}

void bigfile::flush()
{
    assert(is_open());

    if (FlushFileBuffers(pimpl_->file))
        throw std::runtime_error("file flush failed");
}

bigfile::big_t bigfile::size(const std::string& file)
{
    const std::wstring& wstr = utf8_decode(file);
    __stat64 st;
    if (_wstat64(wstr.c_str(), &st))
        throw std::runtime_error("get file size failed");

    return st.st_size;
}

void bigfile::resize(const std::string& file, big_t size)
{
    assert(size > 0);

    const std::wstring& wstr = utf8_decode(file);

    const HANDLE handle = CreateFile(
        wstr.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (handle == INVALID_HANDLE_VALUE)
        throw std::runtime_error("file open error");

    LARGE_INTEGER li;
    li.QuadPart = size;
 
    if (SetFilePointerEx(handle, li, NULL, FILE_BEGIN) == FALSE ||
        SetEndOfFile(handle) == FALSE)
    {
        CloseHandle(handle);
        throw std::runtime_error("file truncate failed");        
    }
    CloseHandle(handle);
}

#elif defined(LINUX_OS)

struct bigfile::impl {
    int fd;

    native_errcode_t open_file(const std::string& filename, unsigned flags, unsigned mode)
    {
        assert(!filename.empty());

        const int fd = ::open(filename.c_str(), flags, mode);
        if (fd == -1)
            return errno;

        if (this->fd)
             CHECK(::close(this->fd), 0);

        this->fd = fd;
        return 0;
    }
};

bigfile::bigfile() : pimpl_(new impl)
{
    pimpl_->fd = 0;
}

bigfile::~bigfile()
{
    close();
}

native_errcode_t bigfile::open(const std::string& file)
{
    return pimpl_->open_file(file, O_RDWR | O_LARGEFILE, 0);
}

native_errcode_t bigfile::append(const std::string& file)
{
    return pimpl_->open_file(file, O_RDWR | O_LARGEFILE | O_CREAT | O_APPEND, 
        S_IRWXU | S_IRGRP | S_IROTH);
}

native_errcode_t bigfile::create(const std::string& file)
{
    return pimpl_->open_file(file, O_RDWR | O_LARGEFILE | O_CREAT | O_TRUNC,
        S_IRWXU | S_IRGRP | S_IROTH);
}

bool bigfile::is_open() const
{
    return (pimpl_->fd != 0);
}

void bigfile::close()
{
    if (!pimpl_->fd)
        return;

    CHECK(::close(pimpl_->fd), 0);

    pimpl_->fd = 0;
}

bigfile::big_t bigfile::position() const
{
    assert(is_open());

    const off64_t pos = lseek64(pimpl_->fd, 0, SEEK_CUR);
    if (pos == (off64_t)-1)
        throw std::runtime_error("get current file position failed (lseek64)");

    return big_t(pos);
}

void bigfile::seek(big_t offset)
{
    assert(is_open());
    assert(offset >= 0);

    const off64_t pos = lseek64(pimpl_->fd, offset, SEEK_SET);
    if (pos == (off64_t)-1)
        throw std::runtime_error("file seek failed (lseek64)");
}

void bigfile::write(const void* data, size_t bytes)
{
    assert(is_open());
    assert(data);
    assert(bytes > 0 && bytes < ((size_t)-1));

    // note that the return value is ssize_t and nbyte is size_t!
    // i guess this is because of the return value is -1 on error. (herp derp..)
    // so the actual number of bytes that can be written is what..
    // std::numeric_limits<size_t>::max() -1 ??
    const size_t ret = ::write(pimpl_->fd, data, bytes);
    if (ret == (size_t)-1)
        throw std::runtime_error("file write failed");
}

size_t bigfile::read(void* buff, size_t bytes)
{
    assert(is_open());
    assert(buff);
    assert(bytes > 0 && bytes < ((size_t)-1));

    const size_t ret = ::read(pimpl_->fd, buff, bytes);
    if (ret == (size_t)-1)
        throw std::runtime_error("file read failed");

    return ret;
}

void bigfile::flush()
{
    assert(is_open());

    if (fdatasync(pimpl_->fd))
        throw std::runtime_error("flush failed");
}

bigfile::big_t bigfile::size(const std::string& file)
{
    struct stat64 st {0};

    if (stat64(file.c_str(), &st))
        throw std::runtime_error("get file size failed (fstat64)");

    return big_t(st.st_size);
}

void bigfile::resize(const std::string& file, big_t size)
{
    assert(size >= 0);

    if (truncate64(file.c_str(), size))
        throw std::runtime_error("file truncate failed");
}

#endif


} // newsflash

