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

#include <chrono>
#include <vector>
#include <string>
#include "types.h"

// this file contains an assortment of platform specific global functions
// and associated types.

namespace newsflash
{
    // convenience wrapper to specify different time intervals.
    template<typename Rep, typename Period>
    void wait_single_object(native_handle_t handle, const std::chrono::duration<Rep, Period>& wait_duration)
    {
        // todo: convert to millisecondstype
    }

    template<typename Rep, typename Period>
    void wait_multiple_objects(const std::vector<native_handle_t>& handles, const std::chrono::duration<Rep, Period>& wait_duration)
    {
        // todo: convert to millisecondstype
    }

    // wait on a single handle indefinitely untill the handle is signaled.
    void wait_single_object(native_handle_t handle);

    // wait on a single handle untill the given ms elapses or the handle is signaled.
    // returns true if object was signaled, otherwise returns false.
    bool wait_single_object(native_handle_t handle, std::chrono::milliseconds ms);

    // wait on a number of handles untill one of them is signaled.
    // returns the handle that was signaled.
    native_handle_t wait_multiple_objects(const std::vector<native_handle_t>& handles);

    // wait on a number of handles untill one of them is signaled or untill
    // the given timespan elapses. 
    // returns the handle that was signaled or invalid handle on timeout
    native_handle_t wait_multiple_objects(const std::vector<native_handle_t>& handles, std::chrono::milliseconds ms);

    inline 
    bool is_signaled(native_handle_t handle)
    {
        // poll the handle
        return wait_single_object(handle, std::chrono::milliseconds(0)); 
    }

    // get a platform provided human readable error string.
    std::string get_error_string(int code);

    // get last platform error code
    native_errcode_t get_last_error();

} // newsflash

