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

namespace newsflash
{
    // waitable handle object
    class waithandle
    {
    public:
        typedef std::vector<waithandle*> list;

        enum class type { socket, event, pipe };

        waithandle(native_handle_t handle, type t, bool r, bool w)
            : handle_(handle), type_(t), read_(r), write_(w)
        {
            assert(read_ || write_);
        }

        // check for readability
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

        static void wait(const list& handles)
        {
            wait_handles(handles, nullptr);
        }
        static bool wait(const list& handles, const std::chrono::milliseconds& ms)
        {
            return wait_handles(handles, &ms);
        }
    private:
        static bool wait_handles(const list& handles, const std::chrono::milliseconds* ms);

        native_handle_t handle_;
        type type_;
        bool read_;
        bool write_;
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


    inline
    void wait(waithandle& h1)
    {
        const waithandle::list handles {
            &h1
        };
        
        waithandle::wait(handles);
    }
    inline
    void wait(waithandle& h1, waithandle& h2)
    {
        const waithandle::list handles {
            &h1, &h2
        };
        
        waithandle::wait(handles);
    }
    inline
    void wait(waithandle& h1, waithandle& h2, waithandle& h3)
    {
        const waithandle::list handles {
            &h1, &h2, &h3
        };
        
        waithandle::wait(handles);
    }    

    inline 
    bool wait(waithandle& h1, const std::chrono::milliseconds& ms)
    {
        const waithandle::list handles {
            &h1
        };
        
        return waithandle::wait(handles, ms);
    }
    inline 
    bool wait(waithandle& h1, waithandle& h2, const std::chrono::milliseconds& ms)
    {
        const waithandle::list handles {
            &h1, &h2
        };

        return waithandle::wait(handles, ms);
    }
    inline 
    bool wait(waithandle& h1, waithandle& h2, waithandle& h3, const std::chrono::milliseconds& ms)
    {
        const waithandle::list handles {
            &h1, &h2, &h3
        }; 

        return waithandle::wait(handles, ms);
    }

} // newsflash
