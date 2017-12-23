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

#include "newsflash/config.h"

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
    HANDLE file = INVALID_HANDLE_VALUE;
    bool append = false;

    impl(const std::string& filename, unsigned flags, std::error_code* error)
    {
        const std::wstring& wide = utf8::decode(filename);

        file = CreateFile(
            wide.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            flags,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if (file == INVALID_HANDLE_VALUE)
        {
            if (error)
            {
                *error = std::error_code(GetLastError(), std::system_category());
                return;
            }
            throw std::system_error(GetLastError(), std::system_category(), "file open failed: " + filename);
        }

        // make the handle non-inheritable so that any child processes
        // that get started by this process do not have these files open
        // todo: this is not really consistent behaviour with linux, should the
        // handles be inherited?
        SetHandleInformation(file, HANDLE_FLAG_INHERIT, 0);
    }
   ~impl()
    {
        if (file != INVALID_HANDLE_VALUE)
        {
            ASSERT(CloseHandle(file) == TRUE);
        }
    }
};

void bigfile::open(const std::string& file, unsigned flags, std::error_code* error)
{
    // note that the open flags _cannot_ be combined!
    unsigned mode = OPEN_EXISTING;
    if (flags & o_create)
        mode = CREATE_ALWAYS;

    std::unique_ptr<impl> p(new impl(file, mode, error));
    if (error && *error)
        return;

    p->append = ((flags & o_append) == o_append);

    const bool truncate = ((flags & o_truncate) == o_truncate);
    if (truncate)
    {
        SetFilePointer(p->file, 0, NULL, FILE_BEGIN);
        SetEndOfFile(p->file);
    }

    pimpl_ = std::move(p);
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

bigfile::big_t bigfile::size() const
{
    assert(is_open());

    LARGE_INTEGER size = {0};
    if (!GetFileSizeEx(pimpl_->file, &size))
        throw std::runtime_error("get file size failed");

    return size.QuadPart;
}

void bigfile::seek(big_t offset)
{
    assert(is_open());
    assert(offset >= 0);

    auto hi = static_cast<LONG>(offset >> 32);
    auto lo = static_cast<LONG>(offset & 0xFFFFFFFF);
    if (SetFilePointer(pimpl_->file, lo, &hi, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
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
        if (SetFilePointer(pimpl_->file, 0, NULL, FILE_END) == INVALID_SET_FILE_POINTER)
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

std::pair<std::error_code, bigfile::big_t> bigfile::size(const std::string& file)
{
    const std::wstring& wstr = utf8::decode(file);
    __stat64 st;
    if (_wstat64(wstr.c_str(), &st))
        return { std::error_code(GetLastError(), std::system_category()), 0 };

    return{ std::error_code(), st.st_size };
}

std::error_code bigfile::erase(const std::string& file)
{
    // todo: if the file is read-only, first remove the read-only attribute

    const std::wstring& wstr = utf8::decode(file);
    if (DeleteFileW(wstr.c_str()) == FALSE)
        return std::error_code(GetLastError(), std::system_category());

    return std::error_code();
}

std::error_code bigfile::resize(const std::string& file, big_t size)
{
    assert(size >= 0);

    const std::wstring& wstr = utf8::decode(file);

    const HANDLE handle = CreateFile(
        wstr.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (handle == INVALID_HANDLE_VALUE)
        return std::error_code(GetLastError(), std::system_category());

    LARGE_INTEGER li;
    li.QuadPart = size;

    if (SetFilePointerEx(handle, li, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER ||
        SetEndOfFile(handle) == FALSE)
    {
        const int err = GetLastError();
        CloseHandle(handle);
        return std::error_code(err, std::system_category());
    }
    CloseHandle(handle);

    return std::error_code();
}

#elif defined(LINUX_OS)

struct bigfile::impl {
    int fd = -1;

    impl(const std::string& filename, unsigned flags, unsigned mode, std::error_code* error)
    {
        fd = ::open(filename.c_str(), flags, mode);
        if (fd == -1)
        {
            if (error)
            {
                *error = std::error_code(errno, std::generic_category());
                return;
            }
            throw std::system_error(errno, std::generic_category(), "file open failed: " + filename);
        }
    }
   ~impl()
    {
        if (fd != -1)
        {
            ASSERT(::close(fd) == 0);
        }
    }
};

void bigfile::open(const std::string& file, unsigned flags, std::error_code* error)
{
    int mode = O_RDWR | O_LARGEFILE;
    if (flags & o_create)
        mode |= O_CREAT;
    if (flags & o_truncate)
        mode |= O_TRUNC;
    if (flags & o_append)
        mode |= O_APPEND;

    std::unique_ptr<impl> p(new impl(file, mode, S_IRWXU | S_IRGRP | S_IROTH, error));
    if (error && *error)
        return;

    pimpl_ = std::move(p);
}

bigfile::big_t bigfile::position() const
{
    assert(is_open());

    const off64_t pos = lseek64(pimpl_->fd, 0, SEEK_CUR);
    if (pos == (off64_t)-1)
        throw std::runtime_error("get current file position failed (lseek64)");

    return big_t(pos);
}

bigfile::big_t bigfile::size() const
{
    assert(is_open());

    struct stat64 st {0};
    if (fstat64(pimpl_->fd, &st))
        throw std::system_error(errno, std::generic_category(),
            "failed to get file size");

    return big_t{st.st_size};
}

void bigfile::seek(big_t offset)
{
    assert(is_open());
    assert(offset >= 0);

    const off64_t pos = lseek64(pimpl_->fd, offset, SEEK_SET);
    if (pos == (off64_t)-1)
        throw std::system_error(errno, std::generic_category(),
            "file seek failed");
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
        throw std::system_error(errno, std::generic_category(),
            "file write failed");
}

size_t bigfile::read(void* buff, size_t bytes)
{
    assert(is_open());
    assert(buff);
    assert(bytes > 0 && bytes < ((size_t)-1));

    const size_t ret = ::read(pimpl_->fd, buff, bytes);
    if (ret == (size_t)-1)
        throw std::system_error(errno, std::generic_category(),
            "file read failed");

    return ret;
}

void bigfile::flush()
{
    assert(is_open());

    if (fdatasync(pimpl_->fd))
        throw std::runtime_error("flush failed");
}

std::pair<std::error_code, bigfile::big_t> bigfile::size(const std::string& file)
{
    struct stat64 st {0};

    if (stat64(file.c_str(), &st))
        return { std::error_code(errno, std::generic_category()), 0};

    return { std::error_code(), big_t(st.st_size) };
}

std::error_code bigfile::erase(const std::string& file)
{
    if (unlink(file.c_str()) == -1)
        return std::error_code(errno, std::generic_category());
    return std::error_code();
}

std::error_code bigfile::resize(const std::string& file, big_t size)
{
    assert(size >= 0);

    if (truncate64(file.c_str(), size))
        return std::error_code(errno, std::generic_category());

    return std::error_code();
}

#endif

// common implementation follows

bigfile::bigfile()
{}

bigfile::bigfile(const std::string& file, unsigned flags)
{
    open(file, flags);
}

bigfile::bigfile(bigfile&& other) : pimpl_(std::move(other.pimpl_))
{}

// have to define this because of the private implementation struct definition is here
bigfile::~bigfile()
{}

bool bigfile::is_open() const
{
    return !!pimpl_;
}

void bigfile::close()
{
    pimpl_.reset();
}

bigfile& bigfile::operator=(bigfile&& other)
{
    if (&other == this)
        return *this;

    pimpl_ = std::move(other.pimpl_);
    return *this;
}

bool bigfile::exists(const std::string& file)
{
    const auto ret = bigfile::size(file);
    if (ret.first == std::error_code())
        return true;

    return false;
}

} // newsflash

