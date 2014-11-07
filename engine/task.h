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
#include <vector>

namespace newsflash
{
    class action;
    class cmdlist;
    struct settings;

    // tasks implement some nntp data based activity in the engine, for
    // example extracting binary content from article data.
    class task 
    {
    public:
        virtual ~task() = default;

        virtual std::unique_ptr<cmdlist> create_commands() = 0;

        virtual std::unique_ptr<action> start() = 0;

        // kill the task. if the task is not complete then this has the effect
        // of canceling all the work that has been done, for example removing
        // any files created on the filesystem etc. if task is complete then
        // simply the non-persistent data is cleaned away. 
        // after this call returns the object can be deleted.
        virtual std::unique_ptr<action> kill() = 0;

        // flush task intermediate state to the disk and make it permanent.
        virtual std::unique_ptr<action> flush() = 0;

        virtual std::unique_ptr<action> finalize() = 0;

        // complete the action. create actions (if any) and store
        // them in the next list
        virtual void complete(action& act,
            std::vector<std::unique_ptr<action>>& next) = 0;

        // complete the cmdlist. create actions (if any) and store
        // them in the next list
        virtual void complete(cmdlist& cmd,
            std::vector<std::unique_ptr<action>>& next) = 0;

        // update task settings
        virtual void configure(const settings& s) = 0;

        virtual double completion() const = 0;


        virtual bool has_commands() const = 0;

    protected:
    private:
    };

} // newsflash