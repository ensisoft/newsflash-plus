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

#include <typeinfo>
#include <memory>
#include <cstddef>
#include <cassert>

namespace msglib
{
    // message wraps safely arbitrary types and provides
    // a safe "type casting" facility.
    class message
    {
    public:
        // construct an empty message.
        message() : id_(0)
        {}

        // construct a message with msg payload and id
        template<typename T>
        message(int id, T msg) : id_(id), msg_(new message::msg<T>(std::move(msg)))
        {}

        message(message&& other) : id_(other.id_), msg_(std::move(other.msg_))
        {}

        // cast to type T object.
        template<typename T>
        T& as()
        {
            void* ptr = msg_->get_if(typeid(T));
            return *static_cast<T*>(ptr);
        }

        // cast to type const T object
        template<typename T>
        const T& as() const
        {
            const void* ptr = msg_->get_if(typeid(T));
            return *static_cast<const T*>(ptr);
        }

        // get message id.
        int id() const
        {
            return id_;
        }

        template<typename T>
        bool test() const
        {
            return msg_->get_if(typeid(T)) != nullptr;
        }

        bool empty() const
        {
            return !msg_;
        }

        message& operator=(message&& other)
        {
            id_  = other.id_;
            msg_ = std::move(other.msg_);
            return *this;
        }

    private:
        struct any_message {
            virtual ~any_message() = default;

            virtual void* get_if(const std::type_info& check) = 0;
        };

        template<typename T>
        struct msg : public any_message {

            msg(T&& data) : data_(std::move(data))
            {}

            virtual void* get_if(const std::type_info& check) override
            {
                if (typeid(T) == check)
                    return &data_;
                return nullptr;
            }
            T data_;
        };

    private:
        int id_;
        std::unique_ptr<any_message> msg_;
    };

} // msglib
