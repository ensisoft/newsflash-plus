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

#if defined(LINUX_OS)
#  include <unistd.h>
#  include <sys/eventfd.h>
#endif
#include <stdexcept>
#include <cassert>
#include "event.h"
#include "assert.h"

namespace newsflash
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
    ASSERT(CloseHandle(pimpl_->handle) == TRUE);
}

native_handle_t event::handle() const
{
    return pimpl_->handle;
}

void event::wait()
{
    const DWORD ret = WaitForSingleObject(pimpl_->handle, INFINITE);
    if (ret == WAIT_FAILED || ret == WAIT_ABANDONED)
        throw std::runtime_error("WaitForSingleObject failed");
}

void event::set()
{
    if (pimpl_->signaled) return;

    ASSERT(SetEvent(pimpl_->handle));

    pimpl_->signaled = true;    
}

void event::reset()
{
    if (!pimpl_->signaled) return;

    ASSERT(ResetEvent(pimpl_->handle));

    pimpl_->signaled = false;
}


#elif defined(LINUX_OS)

struct event::impl {
    int fds[2];
    //int fd;
    bool signaled;
};

event::event() : pimpl_(new impl)
{
    if (pipe(pimpl_->fds) == -1)
         throw std::runtime_error("create pipe failed");
    //pimpl_->fd = eventfd(0, 0);
    //if (pimpl_->fd == -1)
    //    throw std::runtime_error("create event object failed");
    pimpl_->signaled = false;

}

event::~event()
{
    //close(pimpl_->fd);
    //CHECK(close(pimpl_->fd), 0);
    close(pimpl_->fds[0]);
    close(pimpl_->fds[1]);
}

native_handle_t event::handle() const
{
    return pimpl_->fds[0]; // return read end
}

void event::wait()
{
    // this function executes typically in another thread 
    // with respect to the set/reset functions. 
    // so if you want to use signaled_ flag here to avoid
    // a call to select think twice about memory consistency. (and race)

    fd_set read;

    FD_ZERO(&read);
    FD_SET(pimpl_->fds[0], &read);

    if (::select(pimpl_->fds[0] + 1, &read, nullptr, nullptr, nullptr) == -1)
        throw std::runtime_error("select failed");
}

void event::set()
{
    if (pimpl_->signaled) return;

    //uint64_t c = 1;
    char c = 1;
    const int ret = ::write(pimpl_->fds[1], &c, sizeof(c));
    if (ret == -1)
        throw std::runtime_error("failed to set event (pipe write)");

    pimpl_->signaled = true;
}

void event::reset()
{
    if (!pimpl_->signaled) return;

    //uint64_t c = 0;
    char c = 1;
    const int ret = ::read(pimpl_->fds[0], &c, sizeof(c));
    if (ret == -1)
        throw std::runtime_error("failed to reset event (pipe read)");

    pimpl_->signaled = false;
}

#endif

bool event::is_set() const
{
    return pimpl_->signaled;
}

} // newsflash
