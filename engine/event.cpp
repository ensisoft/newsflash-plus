// Copyright (c) 2013 Sami Väisänen, Ensisoft 
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
#  include <unistd.h>
#  include <sys/eventfd.h>
#  include <mutex>
#endif
#include <stdexcept>
#include <cassert>
#include "event.h"

namespace corelib
{

#if defined(WINDOWS_OS)

struct event::impl {
    HANDLE handle;
    bool signaled;
};

event::event() : pimpl_(new impl)
{
    pimpl_->handle = CreateEvent(
        NULL, 
        TRUE, // manual reset
        FALSE, // initially not signaled
        NULL);
    if (pimpl_->handle == NULL)
        throw std::runtime_error("CreateEvent failed");

    pimpl_->signaled = false;
}

event::~event()
{
    CloseHandle(pimpl_->handle);
}

waithandle event::wait() const
{
    return waithandle {pimpl_->handle, waithandle::type::event, true, false};
}

void event::set()
{
    if (pimpl_->signaled) 
        return;

    SetEvent(pimpl_->handle);

    pimpl_->signaled = true;    
}

void event::reset()
{
    if (!pimpl_->signaled) 
        return;

    ResetEvent(pimpl_->handle);

    pimpl_->signaled = false;
}


#elif defined(LINUX_OS)

struct event::impl {
    int fd;
    bool signaled;
    std::mutex mutex;
};

event::event() : pimpl_(new impl)
{
    pimpl_->fd = eventfd(0, 0);
    if (pimpl_->fd == -1)
        throw std::runtime_error("create eventfd failed");

    pimpl_->signaled = false;
}

event::~event()
{
    close(pimpl_->fd);
}

waithandle event::wait() const
{
    return waithandle {pimpl_->fd, waithandle::type::event, true, false};
}

void event::set()
{
    std::lock_guard<std::mutex> lock(pimpl_->mutex);

    if (pimpl_->signaled) 
        return;

    uint64_t c = 1;
    if (::write(pimpl_->fd, &c, sizeof(c)) == -1)
        throw std::runtime_error("event set failed");

    pimpl_->signaled = true;
}

void event::reset()
{
    std::lock_guard<std::mutex> lock(pimpl_->mutex);

    if (!pimpl_->signaled) 
        return;

    uint64_t c = 0;
    if (::read(pimpl_->fd, &c, sizeof(c)) == -1)
        throw std::runtime_error("event reset failed");

    pimpl_->signaled = false;
}

#endif

} // corelib
