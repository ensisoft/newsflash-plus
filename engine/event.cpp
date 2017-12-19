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
#elif defined(LINUX_OS)
#  include <unistd.h>
#  include <sys/eventfd.h>
#  include <mutex>
#endif
#include <stdexcept>
#include <cassert>
#include "assert.h"
#include "event.h"

namespace newsflash
{

#if defined(WINDOWS_OS)

struct Event::impl {
    HANDLE handle;
    bool signaled;
};

Event::Event() : pimpl_(new impl)
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

Event::~Event()
{
    ASSERT(CloseHandle(pimpl_->handle) == TRUE);
}

WaitHandle Event::GetWaitHandle() const
{
    return { pimpl_->handle, WaitHandle::Type::Event, true, false};
}

void Event::SetSignal()
{
    if (pimpl_->signaled)
        return;

    SetEvent(pimpl_->handle);

    pimpl_->signaled = true;
}

void Event::ResetSignal()
{
    if (!pimpl_->signaled)
        return;

    ResetEvent(pimpl_->handle);

    pimpl_->signaled = false;
}

bool Event::IsSignalled() const
{
    const auto ret = WaitForSingleObject(pimpl_->handle, 0);
    if (ret == WAIT_OBJECT_0)
        return true;
    else if (ret == WAIT_TIMEOUT)
        return false;

    throw std::runtime_error("WaitSingleObject failed");
}


#elif defined(LINUX_OS)

struct Event::impl {
    int fd;
    bool signaled;
    std::mutex mutex;
};

Event::Event() : pimpl_(new impl)
{
    pimpl_->fd = eventfd(0, 0);
    if (pimpl_->fd == -1)
        throw std::runtime_error("create eventfd failed");

    pimpl_->signaled = false;
}

Event::~Event()
{
    ASSERT(close(pimpl_->fd) == 0);
}

WaitHandle Event::GetWaitHandle() const
{
    return {pimpl_->fd, WaitHandle::Type::Event, true, false};
}

void Event::SetSignal()
{
    std::lock_guard<std::mutex> lock(pimpl_->mutex);

    if (pimpl_->signaled)
        return;

    uint64_t c = 1;
    if (::write(pimpl_->fd, &c, sizeof(c)) == -1)
        throw std::runtime_error("event set failed");

    pimpl_->signaled = true;
}

void Event::ResetSignal()
{
    std::lock_guard<std::mutex> lock(pimpl_->mutex);

    if (!pimpl_->signaled)
        return;

    uint64_t c = 0;
    if (::read(pimpl_->fd, &c, sizeof(c)) == -1)
        throw std::runtime_error("event reset failed");

    pimpl_->signaled = false;
}

bool Event::IsSignalled() const
{
    std::lock_guard<std::mutex> lock(pimpl_->mutex);
    return pimpl_->signaled;
}

#endif

} // newsflash
