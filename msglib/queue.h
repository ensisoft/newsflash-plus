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

#include <condition_variable>
#include <deque>
#include <mutex>
#include <memory>
#include <utility> // for std::forward
#include <cassert>

namespace msglib
{
    // thread safe message queue. can be used to dispatch
    // messages between two async entities.
    template<typename Message>
    class queue
    {
        struct wait_state {
            std::condition_variable cond;
            std::mutex mutex;
            std::size_t refc;
            bool done;
        };

    public:
        class future 
        {
        public:
            void wait()
            {
                std::unique_lock<std::mutex> lock(wait_->mutex);
                if (wait_->done)
                    return;
                wait_->cond.wait(lock);
            }
            bool ready() const
            {
                std::unique_lock<std::mutex> lock(wait_->mutex);
                return wait_->done;
            }
        private:
            friend class queue;

            std::shared_ptr<wait_state> wait_;
        };

        class message 
        {
        public:
            Message value;

            message()
            {}

            message(const message& other) : value(other.value), wait_(other.wait_)
            {
                if (wait_)
                    wait_->refc++;
            }

            message(const Message& m) : value(m)
            {}

            message(Message&& m) : value(std::move(m))
            {}

           ~message()
            {
                if (wait_ && --wait_->refc == 0)
                    dispose();
            }

            void dispose()
            {
                if (wait_)
                {
                    std::lock_guard<std::mutex> lock(wait_->mutex);
                    wait_->done = true;
                    wait_->cond.notify_one();                
                }
            }

            message& operator=(const message& other)
            {
                message tmp(other);

                std::swap(tmp.wait_, wait_);
                std::swap(tmp.value, value);

                return *this;
            }

        private:
            friend class queue;

            std::shared_ptr<wait_state> wait_;
        };

        // post a message to the back of the queue. 
        // returns immediately and caller cannot determine
        // when the message will be processed by the receiver.
        void post(const Message& msg)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            messages_.push_back(message { msg });
        }

        template<typename... Args>
        void emplace_post(Args&&... args)
        {
            Message m = { std::forward<Args>(args)... };
            std::lock_guard<std::mutex> lock(mutex_);
            messages_.push_back(message { std::move(m) });
        }        

        // send a message to the back of the queue.
        // returns a future object which may be used to block
        // the caller untill the message has been processed by
        // the receiver.
        future send(const Message& msg)
        {
            auto wait = std::make_shared<wait_state>();
            wait->done = false;
            wait->refc = 1;

            message m = { msg };
            m.wait_ = wait;

            future f;
            f.wait_ = wait;

            std::lock_guard<std::mutex> lock(mutex_);
            messages_.push_back(std::move(m));

            return f;
        }

        template<typename... Args>
        future emplace_send(Args&&... args)
        {
            auto wait = std::make_shared<wait_state>();
            wait->done = false;
            wait->refc = 1;

            Message m = { std::forward<Args>(args)... };
            message mm = { std::move(m) };
            mm.wait_ = wait;

            future f;
            f.wait_ = wait;

            std::lock_guard<std::mutex> lock(mutex_);
            messages_.push_back(std::move(mm));
        }


        // get a message from the queue if any. 
        // returns true if message was retrieved, otherwise false.
        bool get(message& m)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (messages_.empty())
                return false;

            m = messages_.front();
            messages_.pop_front();
            return true;
        }

    private:
        std::mutex mutex_;
        std::deque<message> messages_;
    };

} // msglib
