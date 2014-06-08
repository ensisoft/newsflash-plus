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

#include <vector>
#include <cassert>
#include <memory>
#include "message.h"
#include "message_register.h"

namespace sdk
{
    class message_dispatch
    {
    public:
        bool post(std::shared_ptr<message> msg)
        {
            for (auto& sink : sinks_)
            {
                sink->dispatch(msg);
            }
            return true;
        }

        template<typename Msg, typename Receiver>
        void listen(Receiver* recv) 
        {
            const std::size_t id = message_register::find<Msg>();
            assert(id && "type is not registered");

            using sink_type = any_sink<Receiver, Msg>;

            auto sink  = std::make_shared<sink_type>();
            sink->recv = recv;
            sinks_.push_back(sink);
        }

        static 
        message_dispatch& get();

    private:
        message_dispatch()
        {}

    private:
        struct sink {
            virtual ~sink() = default;
            virtual void dispatch(std::shared_ptr<message>& msg) = 0;
        };

        template<typename R, typename T>
        struct any_sink : public sink {
            R* recv;

            virtual void dispatch(std::shared_ptr<message>& msg) override
            {
                static auto id = message_register::find<T>();
                if (msg->identity() != id)
                    return;
                std::shared_ptr<T> down = std::static_pointer_cast<T>(msg);
                recv->on_message(down);
            }
        };

        std::vector<std::shared_ptr<sink>> sinks_;
    };

} // sdk
