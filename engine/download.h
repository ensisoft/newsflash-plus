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
#include <vector>
#include <string>
#include <deque>
#include "task.h"
#include "bigfile.h"
#include "encoding.h"
#include "decoder.h"

namespace newsflash
{
    namespace ui {
        class file;
        class download;
    } // ui

    // extract encoded content from the buffers
    class download : public task
    {
    public:
        download(std::size_t id, std::size_t account, const ui::download& details);
       ~download();

        virtual void start() override;

        virtual void kill() override;

        virtual void flush() override;

        virtual void pause() override;

        virtual void execute() override;

        virtual void complete(std::unique_ptr<action> act);

        virtual void complete(std::unique_ptr<cmdlist> cmd);

        virtual ui::task get_ui_state() const override
        { return state_; }

        virtual std::size_t get_id() const 
        { return id_; }

        virtual std::size_t get_account() const
        { return account_; }

        virtual state get_state() const 
        { return state_.st; }

        virtual error get_error() const 
        { return state_.err; }

    private:
        class decode;
        class write;
        class bodylist;

    private:
        std::size_t id_;
        std::size_t account_;
        std::string path_;
        std::string name_;
        std::deque<std::string> groups_;
        std::deque<std::string> articles_;
    private:
        bool overwrite_;
        bool keeptext_;
    private:
        ui::task state_;
    };

} // newsflash

