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

#pragma once

#include "newsflash/config.h"

#include <chrono>
#include <vector>
#include <cassert>

#include "native_types.h"

namespace newsflash
{
    // waitable handle object. the handles are not resuable and should
    // only be used in a single call to WaitForMultipleHandles(...). after that a new
    // waitable handle needs to be queried from the object.
    class WaitHandle
    {
    public:
        typedef std::vector<WaitHandle*> WaitList;

        enum class Type {
            // handle is a socket
            Socket,
            // handle is an event object
            Event,
            // handle is a pipe
            pipe
        };

        WaitHandle(native_handle_t handle, Type type, bool read, bool write)
            : handle_(handle), type_(type), read_(read), write_(write)
        {
            assert(read || write);
        }
        WaitHandle(native_handle_t handle, native_socket_t sock, bool read, bool write)
            : handle_(handle), socket_(sock), type_(Type::Socket), read_(read), write_(write)
        {
            assert(read || write);
        }

        // check for readability. this is always available.
        bool CanRead() const
        {
            return read_;
        }

        // check for writability.
        // this is only available when the handle refers to a socket.
        bool CanWrite() const
        {
            assert(type_ == Type::Socket);
            return write_;
        }

        operator bool() const
        {
            return CanRead();
        }


        // WaitForMultipleHandles indefinitely for the listed handles.
        // returns when any handle becomes signaled.
        static void WaitForMultipleHandles(const WaitList& handles)
        {
            WaitForMultipleHandles(handles, nullptr);
        }

        // WaitForMultipleHandles until the specified milliseconds elapses
        // or any of the listed handles becomes signaled.
        // if timeout occurs returns false, otherwise true
        // and the handles need to be checked which one is
        // in signaled state.
        static bool WaitForMultipleHandles(const WaitList& handles, const std::chrono::milliseconds& ms)
        {
            return WaitForMultipleHandles(handles, &ms);
        }
    private:
        static bool WaitForMultipleHandles(const WaitList& handles, const std::chrono::milliseconds* ms);
    private:
        native_handle_t handle_ = 0;
        native_socket_t socket_ = 0;
        Type type_ = Type::Socket;
        bool read_ = false;
        bool write_ = false;
    };

    template<typename T>
    void WaitForSingleObject(const T& obj)
    {
        auto handle = obj.GetWaitHandle();

        const WaitHandle::WaitList handles {
            &handle
        };
        WaitHandle::WaitForMultipleHandles(handles);
    }

    template<typename T, typename F>
    void WaitForMultipleObjects(const T& obj1, const F& obj2)
    {
        auto h1 = obj1.GetWaitHandle();
        auto h2 = obj2.GetWaitHandle();
        const WaitHandle::WaitList handles {
            &h1, &h2
        };
        WaitHandle::WaitForMultipleHandles(handles);
    }
    template<typename T, typename F, typename U>
    void WaitForMultipleObjects(const T& obj1, const F& obj2, const U& obj3)
    {
        auto h1 = obj1.GetWaitHandle();
        auto h2 = obj2.GetWaitHandle();
        auto h3 = obj3.GetWaitHandle();
        const WaitHandle::WaitList handles {
            &h1, &h2, &h3
        };
        WaitHandle::WaitForMultipleHandles(handles);
    }

    template<typename T>
    bool IsSignaled(const T& obj)
    {
        auto handle = obj.GetWaitHandle();

        const WaitHandle::WaitList handles { &handle };

        return WaitHandle::WaitForMultipleHandles(handles, std::chrono::milliseconds(0));
    }

    inline
    void WaitForSingleHandle(WaitHandle& h1)
    {
        const WaitHandle::WaitList handles { &h1 };

        WaitHandle::WaitForMultipleHandles(handles);
    }
    inline
    void WaitForMultipleHandles(WaitHandle& h1, WaitHandle& h2)
    {
        const WaitHandle::WaitList handles { &h1, &h2 };

        WaitHandle::WaitForMultipleHandles(handles);
    }
    inline
    void WaitForMultipleHandles(WaitHandle& h1, WaitHandle& h2, WaitHandle& h3)
    {
        const WaitHandle::WaitList handles { &h1, &h2, &h3 };

        WaitHandle::WaitForMultipleHandles(handles);
    }

    inline
    void WaitForMultipleHandles(WaitHandle& h1, WaitHandle& h2, WaitHandle& h3, WaitHandle& h4)
    {
        const WaitHandle::WaitList handles { &h1, &h2, &h3, &h4 };

        WaitHandle::WaitForMultipleHandles(handles);
    }

    inline
    bool WaitForSingleHandle(WaitHandle& h1, const std::chrono::milliseconds& ms)
    {
        const WaitHandle::WaitList handles { &h1 };

        return WaitHandle::WaitForMultipleHandles(handles, ms);
    }
    inline
    bool WaitForMultipleHandles(WaitHandle& h1, WaitHandle& h2, const std::chrono::milliseconds& ms)
    {
        const WaitHandle::WaitList handles { &h1, &h2 };

        return WaitHandle::WaitForMultipleHandles(handles, ms);
    }
    inline
    bool WaitForMultipleHandles(WaitHandle& h1, WaitHandle& h2, WaitHandle& h3, const std::chrono::milliseconds& ms)
    {
        const WaitHandle::WaitList handles { &h1, &h2, &h3 };

        return WaitHandle::WaitForMultipleHandles(handles, ms);
    }

    inline
    bool WaitForMultipleHandles(WaitHandle& h1, WaitHandle&h2, WaitHandle& h3, WaitHandle& h4, const std::chrono::milliseconds& ms)
    {
        const WaitHandle::WaitList handles { &h1, &h2, &h3, &h4 };

        return WaitHandle::WaitForMultipleHandles(handles, ms);
    }

} // newsflash
