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

#pragma once

#include <newsflash/config.h>
#include <memory>

// collection of generic usable utility functions

namespace newsflash
{

    template<typename Handle, typename Deleter>
    class unique_handle 
    {
    public:
        unique_handle(Handle handle, const Deleter& deleter) 
            : handle_(handle), deleter_(deleter)
        {}

        unique_handle(unique_handle&& other) 
            : handle_(other.handle_), deleter_(other.deleter_)
        {
            other.handle_ = Handle();
        }

       ~unique_handle()
        {
            if (handle_ != Handle())
                deleter_(handle_);
        }
        unique_handle& operator=(unique_handle&& other) NOTHROW
        {
            if (this == &other)
                return *this;

            deleter_(handle_);
            handle_  = other.handle_;
            deleter_ = other.deleter_;

            other.handle_ = Handle();
            return *this;
        }
        Handle get() const
        {
            return handle_;
        }
        Handle release(Handle val = Handle())
        {
            const Handle ret = handle_;
            handle_ = val;
            return ret;
        }

        unique_handle(unique_handle&) = delete;
        unique_handle& operator=(unique_handle&) = delete;

    private:
        Handle handle_;                
        const Deleter deleter_;        
    };

    template<typename Handle, typename Deleter>
    bool operator == (const unique_handle<Handle, Deleter>& handle, Handle value)
    {
        return handle.get() == value;
    }


    template<typename T, typename Deleter>
    unique_handle<T, Deleter> make_unique_handle(T handle, Deleter del)
    {
        return unique_handle<T, Deleter>(handle, del);
    }

    template<typename T, typename Deleter>
    std::unique_ptr<T, Deleter> make_unique_ptr(T* ptr, Deleter del)
    {
        return std::unique_ptr<T, Deleter>(ptr, del);
    }

} // namespace