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

#include <memory>
#include <map>

#include "assert.h"

// collection of generic usable utility functions

namespace newsflash
{

    class noncopyable
    {
        noncopyable(const noncopyable&) = delete;

        noncopyable& operator=(const noncopyable&) = delete;
    protected:
         noncopyable() {}
        ~noncopyable() {}
    };


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


    inline
    std::size_t GB(std::size_t gigs)
    { return gigs * 1024 * 1024 * 1024; }


    inline
    std::size_t MB(std::size_t megs)
    { return megs * 1024 * 1024; }

    inline
    std::size_t KB(std::size_t kbs)
    { return kbs * 1024; }

    struct recursion_detector
    {
        std::map<void*, bool> instance_entry_map;
        struct method_entry {
            method_entry(recursion_detector* detector, void* instance) : detector(detector), instance(instance)
            {
                auto it = detector->instance_entry_map.find(instance);
                ASSERT(it == std::end(detector->instance_entry_map));
                detector->instance_entry_map.insert(std::make_pair(instance, true));
            }
           ~method_entry()
            {
                detector->instance_entry_map.erase(instance);
            }
            recursion_detector* detector = nullptr;
            void* instance = nullptr;
            method_entry(const method_entry&) = delete;
            method_entry& operator=(const method_entry&) = delete;
        };
    };

} // namespace

#define NEWSFLASH_NO_RECURSION_GUARD(instance)    \
    static recursion_detector rd; \
    recursion_detector::method_entry recursion_entry(&rd, instance)
