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

#include <condition_variable>
#include <typeinfo>
#include <mutex>
#include <cassert>
#include "event.h"

namespace newsflash
{
    // FIFO message queue, each message has a particular
    // type that is identified by id.
    class msgqueue
    {
    public:
        // arbitrary message type
        class message 
        {
        public:
            message(size_t id) : id_(id), next_(nullptr)
            {}

            virtual ~message() {}

            // get message type/id
            size_t id() const NOTHROW
            {
                return id_;
            }

            // cast to the payload type
            template<typename T>
            const T& as() const NOTHROW
            {
                const void* ptr = get_if(typeid(T));

                assert(ptr && "contained object doesn't seem to have the right type");

                return *static_cast<const T*>(ptr);
            }

            // cast to the const payload type
            template<typename T>
            T& as() NOTHROW
            {
                void* ptr = get_if(typeid(T));

                assert(ptr && "contained object doesn't seem to have the right type");

                return *static_cast<T*>(ptr);
            }

            // test against the given type
            template<typename T>
            bool type_test() const NOTHROW
            {
                void* ptr = get_if(typeid(T));
                return ptr ? true : false;
            }

        protected:
            virtual void* get_if(const std::type_info& other) const = 0;

        private:
            friend class msgqueue;

            const size_t id_;

            message* next_;

        };

        msgqueue() : head_(nullptr), tail_(nullptr), size_(0)
        {}

       ~msgqueue()
        {
            while (head_)
            {
                head_ = head_->next_;
                delete head_;
            }
        }

        template<typename T>
        void send(size_t id, const T& msg)
        {
            // synchronization primitives
            std::mutex m;
            std::condition_variable c;
            std::unique_lock<std::mutex> lock(m);

            // spurious wakeup condition flag
            bool done = false;

            message* next = new send_message<T>(id, m, c, done, msg);

            push_back(next);

            // block the sender untill the message has been processed.
            while (!done)
                c.wait(lock);
        }

        // send a new message to the queue and block untill
        // the message has been processed by the receiving thread.
        template<typename T>
        void send(size_t id, T&& msg)
        {
            typedef typename std::remove_reference<T>::type non_ref_type;

            // synchronization primitives
            std::mutex m;
            std::condition_variable c;
            std::unique_lock<std::mutex> lock(m);
            
            // spurious wakeup condition flag
            bool done = false;
            
            message* next = new send_message<non_ref_type>(id, m, c, done, std::forward<T>(msg));

            // push the message to the queue
            push_back(next);

            // block the sending thread here untill the
            // message has been processed by the receiving thread
            while (!done)
                c.wait(lock);
        }

        // post a new message to the queue and return.
        template<typename T>
        void post(size_t id, const T& msg)
        {
            message* next = new post_message<T>(id, msg);

            push_back(next);
        }

        // post a new message to the queue and return.
        template<typename T>
        void post(size_t id, T&& msg)
        {
            typedef typename std::remove_reference<T>::type non_ref_type;

            // in a template T&& can be either an lvalue ref or
            // an rvalue ref. if it's an lvalue we don't want to 
            // move anything out of it.
            message* next = new post_message<non_ref_type>(id, std::forward<T>(msg));

            push_back(next);
        }

        // try to get a message from the queue returning immediately.
        // returns true if message was available or otherwise false.
        bool try_get(std::unique_ptr<message>& msg) NOTHROW
        {
            std::unique_lock<std::mutex> lock(mutex_);

            if (head_ == nullptr)
                return false;

            msg = std::unique_ptr<message>(pop_head());

            return true;
        }

        // get a message. blocks untill a message is available.
        std::unique_ptr<message> get() NOTHROW
        {
            std::unique_lock<std::mutex> lock(mutex_);

            // spurious wakeup condition check
            while (head_ == nullptr)
                cond_.wait(lock);

            return std::unique_ptr<message>(pop_head());
        }

        size_t size() const NOTHROW
        {
            std::lock_guard<std::mutex> lock(mutex_);

            return size_;
        }

        handle_t handle() const  NOTHROW
        {
            return event_.handle();
        }
    private:
        template<typename T>
        class send_message : public message
        {
        public:
            send_message(size_t id, std::mutex& m, std::condition_variable& c, bool& done, const T& data) : 
                message(id), mutex_(m), cond_(c), done_(done), 
                data_(data)
            {}

            send_message(size_t id, std::mutex& m, std::condition_variable& c, bool& done, T&& ref) :
                message(id), mutex_(m), cond_(c), done_(done), 
                data_(std::move(ref))
            {}

           ~send_message()
            {
                // acquire a lock on the mutex, this won't succeed
                // untill the sending thread has gone into condition wait
                // and given up the mutex. this makes sure that 
                // the waiter cannot miss the signal below
                std::lock_guard<std::mutex> lock(mutex_);

                done_ = true;

                // signal the sender
                cond_.notify_one();
            }
        protected:
            void* get_if(const std::type_info& check) const
            {
                if (typeid(T) != check)
                    return nullptr;
                return (void*)&data_;
            }
        private:
            std::mutex& mutex_;
            std::condition_variable& cond_;
            bool& done_;
            T data_;            
        };

        template<typename T>
        class post_message : public message
        {
        public:
            post_message(size_t id, const T& data) : message(id), data_(data)
            {}
            post_message(size_t id, T&& ref) : message(id), data_(std::move(ref))
            {}
        protected:
            void* get_if(const std::type_info& check) const
            {
                if (typeid(T) != check)
                    return nullptr;
                return (void*)&data_;
            }
        private:
            T data_;
        };

        message* pop_head() 
        {
            // get current head and move on to next message in the list
            // caller is expected to hold the lock

            message* head = head_;

            head_ = head_->next_;

            if (head_ == nullptr)
                tail_ = nullptr;

            if (--size_ == 0)
                event_.reset();

            return head;
        }

        void push_back(message* m) NOTHROW
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (tail_)
            {
                tail_->next_ = m;
                tail_ = m;
            }
            else
            {
                tail_ = head_ = m;
            }
            cond_.notify_one();

            if (++size_ == 1)
                event_.set();
        }

        mutable std::mutex mutex_;
        std::condition_variable cond_;
        message* head_;
        message* tail_;
        size_t size_;
        event event_;
    };

} // namespace

