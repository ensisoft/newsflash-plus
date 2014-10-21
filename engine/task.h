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

#include <functional>
#include <memory>
#include "ui/task.h"

namespace newsflash
{
    namespace ui {
        struct error;
    }

    class action;
    class cmdlist;
    struct settings;

    // tasks implement some nntp data based activity in the engine, for
    // example extracting binary content from article data.
    class task 
    {
    public:
        using state = ui::task::state;

        // this callback is invoked when theres a new action to be performed.
        std::function<void (std::unique_ptr<action>)> on_action;

        // this callback is invoked when an error occurs.
        // the error object carries more information
        std::function<void (const ui::error&)> on_error;

        virtual ~task() = default;

        // start the task.
        // precondition: task is queued
        // postcondition: transitions the task from queued to waiting state
        virtual void start() = 0;

        // kill the task. if the task is not complete then this has the effect
        // of canceling all the work that has been done, for example removing
        // any files created on the filesystem etc. if task is complete then
        // simply the non-persistent data is cleaned away. 
        // after this call returns the object can be deleted.
        virtual void kill() = 0;

        //
        virtual void flush() = 0;

        // transition the task to paused state.
        // precondition: task is neither paused, complete or errored
        // postcondition: task is paused
        virtual void pause() = 0;

        // resume a previously paused task.
        // precondition: task is paused
        // postcondition: task is queued if it hasn't run at all. otherwise waiting.
        virtual void resume() = 0;

        //
        virtual bool get_next_cmdlist(std::unique_ptr<cmdlist>& cmds) = 0;

        virtual void complete(std::unique_ptr<action> act) = 0;

        virtual void complete(std::unique_ptr<cmdlist> cmd) = 0;

        virtual void configure(const settings& s) = 0;

        virtual std::size_t get_id() const = 0;

        virtual state get_state() const = 0;

        virtual ui::task get_ui_state() const = 0;        
    protected:
    private:
    };

} // newsflash