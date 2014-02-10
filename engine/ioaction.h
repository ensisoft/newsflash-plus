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

#include <exception>
#include <memory>
#include <string>
#include "buffer.h"
#include "taskio.h"

namespace newsflash
{

    // abstract IO action to be performed on the taskio
    class ioaction 
    {
    public:
        enum class iotype {
            prepare, receive, cancel, flush, finalize
        };

        ioaction(std::size_t id, 
                 std::size_t task, 
                 std::shared_ptr<taskio> io) 
        : id_(id), taskid_(task), io_(io)
        {}

        virtual ~ioaction() = default;

        // perform the IO action
        void perform()
        {
            try
            {
                perform_action();
            }
            catch (const std::exception& e)
            {
                exception_ = std::current_exception();
            }
        }

        void verify()
        {
            if (exception_ == std::exception_ptr())
                return;

            std::rethrow_exception(exception_);
        }


        std::size_t id() const
        {
            return id_;
        }
        std::size_t task() const
        {
            return taskid_;
        }


        // get the actual IO action type
        virtual iotype type() const = 0;

    private:
        const std::size_t id_;
        const std::size_t taskid_;
        std::exception_ptr exception_;

    protected:
        virtual void perform_action() = 0;

    protected:
        std::shared_ptr<taskio> io_;

    };


    // call prepare() 
    class io_prepare : public ioaction
    {
    public:
        io_prepare(std::size_t id, 
                   std::size_t task, 
                   std::shared_ptr<taskio> io) 
        : ioaction(id, task, io)
        {}


        virtual void perform_action() override
        {
            io_->prepare();
        }
        virtual iotype type() const 
        {
            return iotype::prepare;
        }
    private:
    };

    // calls receive(buff)
    class io_receive : public ioaction
    {
    public:
        io_receive(std::size_t id,
                   std::size_t task,
                   std::shared_ptr<taskio> io,
                   std::shared_ptr<buffer> buff)
        : ioaction(id, task, io), buff_(buff)
        {}

        virtual void perform_action() override
        {
            io_->receive(*buff_);
        }
        virtual iotype type() const override
        {
            return iotype::receive;
        }
    private:
        std::shared_ptr<buffer> buff_;
    };

    // calls cancel()
    class io_cancel : public ioaction
    {
    public:
        io_cancel(std::size_t id, 
                  std::size_t task,
                  std::shared_ptr<taskio> io)
        : ioaction(id, task, io)
        {}

        virtual void perform_action() override
        {
            io_->cancel();
        }
        virtual iotype type() const override
        {
            return iotype::cancel;
        }
    private:
    };


    // calls flush()
    class io_flush : public ioaction
    {
    public:
        io_flush(std::size_t id, 
                  std::size_t task,
                  std::shared_ptr<taskio> io)
        : ioaction(id, task, io)
        {}

        virtual void perform_action() override
        {
            io_->flush();
        }
        virtual iotype type() const override
        {
            return iotype::flush;
        }
    private:
    };


    // calls finalize()
    class io_finalize : public ioaction
    {
    public:
        io_finalize(std::size_t id, 
                  std::size_t task,
                  std::shared_ptr<taskio> io)
        : ioaction(id, task, io)
        {}

        virtual void perform_action() override
        {
            io_->finalize();
        }
        virtual iotype type() const override
        {
            return iotype::finalize;
        }
    private:
    };

} // newsflash

