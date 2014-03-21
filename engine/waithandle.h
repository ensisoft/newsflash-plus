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


#pragma once

#include <newsflash/config.h>

#include <chrono>
#include <vector>
#include <cassert>
#include "types.h"

namespace engine
{
    // waitable handle object. the handles are not resuable and should
    // only be used in a single call to wait(...). after that a new
    // waitable handle needs to be queried from the object.
    class waithandle
    {
    public:
        typedef std::vector<waithandle*> list;

        enum class type { socket, event, pipe };

        waithandle(native_handle_t handle, type t, bool r, bool w)
            : handle_(handle), type_(t), read_(r), write_(w)
        {
            assert(r || w); 
            extra.socket_ = 0;
        }
        waithandle(native_handle_t handle, native_socket_t sock, bool r, bool w)
            : handle_(handle), type_(type::socket), read_(r), write_(w)
        {
            assert(r || w);
            extra.socket_ = sock;
        }

        // check for readability. this is always available.
        bool read() const
        {
            return read_;
        }

        // check for writability. 
        // this is only available when the handle refers to a socket.
        bool write() const
        {
            assert(type_ == type::socket);
            return write_;
        }

        operator bool() const
        {
            return read();
        }


        // wait indefinitely for the listed handles.
        // returns when any handle becomes signaled. 
        static 
        void wait(const list& handles)
        {
            wait_handles(handles, nullptr);
        }

        // wait untill the specified milliseconds elapses
        // or any of the listed handles becomes signaled.
        // if timeout occurs returns false, otherwise true
        // and the handles need to be checked which one is
        // in signaled state.
        static 
        bool wait(const list& handles, const std::chrono::milliseconds& ms)
        {
            return wait_handles(handles, &ms);
        }
    private:
        static 
        bool wait_handles(const list& handles, const std::chrono::milliseconds* ms);

        native_handle_t handle_;
        type type_;
        bool read_;
        bool write_;
        union {
            native_socket_t socket_;
        } extra ;
    };

    template<typename T>
    void wait(const T& obj)
    {
        auto handle = obj.wait();
        
        const waithandle::list handles {
            &handle
        };
        waithandle::wait(handles);
    }

    template<typename T>
    bool is_set(const T& obj)
    {
        auto handle = obj.wait();

        const waithandle::list handles { &handle };

        return waithandle::wait(handles, std::chrono::milliseconds(0));
    }

    inline
    void wait_for(waithandle& h1)
    {
        const waithandle::list handles { &h1 };
        
        waithandle::wait(handles);
    }
    inline
    void wait_for(waithandle& h1, waithandle& h2)
    {
        const waithandle::list handles { &h1, &h2 };
        
        waithandle::wait(handles);
    }
    inline
    void wait_for(waithandle& h1, waithandle& h2, waithandle& h3)
    {
        const waithandle::list handles { &h1, &h2, &h3 };
        
        waithandle::wait(handles);
    }    

    inline
    void wait_for(waithandle& h1, waithandle& h2, waithandle& h3, waithandle& h4)
    {
        const waithandle::list handles { &h1, &h2, &h3, &h4 };

        waithandle::wait(handles);
    }

    inline 
    bool wait_for(waithandle& h1, const std::chrono::milliseconds& ms)
    {
        const waithandle::list handles { &h1 };
        
        return waithandle::wait(handles, ms);
    }
    inline 
    bool wait_for(waithandle& h1, waithandle& h2, const std::chrono::milliseconds& ms)
    {
        const waithandle::list handles { &h1, &h2 };

        return waithandle::wait(handles, ms);
    }
    inline 
    bool wait_for(waithandle& h1, waithandle& h2, waithandle& h3, const std::chrono::milliseconds& ms)
    {
        const waithandle::list handles { &h1, &h2, &h3 }; 

        return waithandle::wait(handles, ms);
    }

    inline
    bool wait_for(waithandle& h1, waithandle&h2, waithandle& h3, waithandle& h4,
        const std::chrono::milliseconds& ms)
    {
        const waithandle::list handles { &h1, &h2, &h3, &h4 };

        return waithandle::wait(handles, ms);
    }

} // engine
