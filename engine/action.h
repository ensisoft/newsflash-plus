// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
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
#include "logging.h"

namespace newsflash
{
    // action class encapsulates a single state transition
    // when the action completes successfully and is returned 
    // to the originating object the object is expected to complete
    // a state transition to another state succesfully.
    class action
    {
    public:
        enum class affinity {
            // dispatch the action to any available thread
            // this means that multiple actions from the same originating
            // object may complete in any order.
            any_thread, 

            // dispatch the action to a single thread with affinity to the
            // action id. this means that all actions with single_thread
            // affinity and with the same id will execute in the same thread.
            single_thread
        };

        virtual ~action() = default;

        action(affinity a = affinity::any_thread) : id_(0), affinity_(a)
        {}

        // return wheather an exception happened in perform()
        bool has_exception() const 
        { 
            return exptr_ != std::exception_ptr(); 
        }

        // if theres a captured exception throw it
        void rethrow() const 
        {
            std::rethrow_exception(exptr_);
        }

        // perform the action
        void perform()
        {
            set_thread_log(log_.get());
            try
            {
                xperform();
            }
            catch (const std::exception& e)
            {
                exptr_ = std::current_exception();
                LOG_E(e.what());
                log_->flush();
            }
            set_thread_log(nullptr);
        }

        affinity get_affinity() const 
        { return affinity_; }

        // get action object id.
        std::size_t get_id() const 
        { return id_; }

        // set the action id
        void set_id(std::size_t id) 
        { id_ = id;}

        // set the thread affinity.
        void set_affinity(affinity aff)
        { affinity_ = aff; }

        // set the logger object to be used for this action.
        void set_log(std::shared_ptr<logger> out)
        { log_ = out; }

    protected:
        virtual void xperform() = 0;        

    private:
        std::exception_ptr exptr_;
        std::size_t id_;
        std::shared_ptr<logger> log_;
    private:
        affinity affinity_;
    };
    
} // newsflash
