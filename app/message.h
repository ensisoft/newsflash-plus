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

#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include "typereg.h"

namespace app
{
    class message_dispatcher
    {
    public:
        message_dispatcher()
        {}

        template<typename T>
        void send(const char* receiver, const char* sender, T& msg)
        {
            //DEBUG(str(""))

            static auto id = typereg::insert<T>();

            for (auto& sink : sinks_)
            {
                sink->dispatch(receiver, sender, &msg, id);
            }
        }

        template<typename T, typename Receiver>
        void listen(Receiver* recv, std::string name) 
        {
            typereg::insert<T>();

            using sink_type = any_sink<Receiver, T>;

            auto sink  = std::unique_ptr<sink_type>(new sink_type);
            sink->recv = recv;
            sink->name = std::move(name);
            sinks_.push_back(std::move(sink));
        }

        template<typename Receiver>
        void remove(Receiver* recv)
        {
            sinks_.erase(std::remove_if(std::begin(sinks_), std::end(sinks_), 
                [=](const std::unique_ptr<sink>& test) {
                    return test->is_receiver_match(recv);
                }),
            sinks_.end());
        }

        static 
        message_dispatcher& get();

    private:
        struct sink {
            virtual ~sink() = default;
            virtual bool dispatch(const char* receiver, const char* sender, void* msg, std::size_t tid) = 0;
            virtual bool is_receiver_match(void* recv) const = 0;
        };

        template<typename R, typename T>
        struct any_sink : public sink {
            R* recv;
            std::string name;

            virtual bool dispatch(const char* receiver, const char* sender, void* msg, std::size_t tid)
            {
                static auto id = typereg::find<T>();
                if (tid != id)
                    return false;
                if (receiver && receiver != name)
                    return false;

                T* down = static_cast<T*>(msg);
                recv->on_message(sender, *down);
                return true;
            }
            virtual bool is_receiver_match(void* recv) const
            {
                return recv == this->recv;
            }
        };

        std::vector<std::unique_ptr<sink>> sinks_;
    };

    template<typename T>
    void send(T msg, const char* sender, const char* receiver = nullptr)
    {
        auto& dispatcher = app::message_dispatcher::get();
        dispatcher.send(receiver, sender, msg); 
    }

    template<typename T>
    void send(T* msg, const char* sender, const char* receiver)
    {
        auto& dispatcher = app::message_dispatcher::get();
        dispatcher.send(receiver, sender, *msg);

    }


    template<typename T, typename Recv>
    void listen(Recv* receiver)
    {
        auto& dispatcher = app::message_dispatcher::get();
        dispatcher.listen<T>(receiver, "");
    }

    template<typename T, typename Recv>
    void listen(Recv* receiver, const char* name)
    {
        auto& dispatcher = app::message_dispatcher::get();
        dispatcher.listen<T>(receiver, name);
    }

    template<typename Recv>
    void remove_listener(Recv* receiver)
    {
        auto& dispatcher = app::message_dispatcher::get();
        dispatcher.remove(receiver);
    }

} // app
