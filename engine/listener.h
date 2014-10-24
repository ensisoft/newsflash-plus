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
#include <memory>
#include "ui/task.h"
#include "ui/file.h"
#include "ui/error.h"
#include "ui/connection.h"

namespace newsflash
{
    class cmdlist;
    class action;
    class task;

    // listener interface for listening to task events
    class listener
    {
    public:
        virtual ~listener() = default;

        // an error has occurred. 
        virtual void on_error(task* t, const ui::error& error) = 0;

        // a file completed
        virtual void on_file_complete(task* t, const ui::file& file) = 0;

        virtual void on_state_change(task* t, ui::task::state old, ui::task::state now) = 0;

        virtual void on_execute(task* t, std::unique_ptr<action> a) = 0;

        virtual void on_execute(task* t, std::unique_ptr<cmdlist> c) = 0;
    protected:
    private:

    };

} // engine
