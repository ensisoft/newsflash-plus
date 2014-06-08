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

#include <cstddef>
#include "message_register.h"

namespace sdk
{
    // message posting protocol between components
    class message
    {
    public:
        enum class status {
            none, accept, reject, error
        };
        message(std::size_t id) : id_(id), status_(status::none)
        {}

        void accept()
        {
            status_ = status::accept;
        }
        void reject()
        {
            status_ = status::reject;
        }
        void fail()
        {
            status_ = status::error;
        }
        std::size_t identity() const
        {
            return id_;
        }
    private:
        std::size_t id_;
        status status_;
    };

    // a shim to automatically perform type registration
    template<typename T>
    class msgbase : public message
    {
    protected:
        msgbase() : message(this_type.identity())
        {}
    private:
        static message_register::registrant<T> this_type;
    };

    template<typename T>
    message_register::registrant<T> msgbase<T>::this_type;

} // sdk
