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
#include <string>
#include <deque>
#include <map>
#include "download_state_machine.h"
#include "stopwatch.h"
#include "etacalc.h"
#include "task.h"
#include "bitflag.h"
#include "ui/task.h"

namespace newsflash
{
    namespace ui {
        struct file;
        struct download;
    } // ui

    class decode;
    class bodylist;

    // extract encoded content from the buffers
    class download : public task
    {
    public:
        // this callback is invoked when a new file has been completed.
        std::function<void (const ui::file& file)> on_file;

        download(std::size_t id, std::size_t batch, std::size_t account, const ui::download& details);
       ~download();

        virtual void start() override;

        virtual void kill() override;

        virtual void flush() override;

        virtual void pause() override;

        virtual void resume() override;

        virtual bool get_next_cmdlist(std::unique_ptr<cmdlist>& cmds) override;

        virtual void complete(std::unique_ptr<action> act) override;

        virtual void complete(std::unique_ptr<cmdlist> cmd) override;

        virtual void configure(const ui::settings& settings) override;

        virtual ui::task get_ui_state() const override;

        virtual std::size_t get_id() const 
        { return task_id_; }

        virtual state get_state() const 
        { return stm_.get_state(); }

    private:
        class file;
        class write;

    private:
        void complete(decode& d);
        void complete(write& w);

    private:
        std::size_t task_id_;
        std::size_t batch_id_;
        std::size_t main_account_;
        std::size_t fill_account_;
        std::size_t expected_size_;
        std::string path_;
        std::string desc_;
    private:
        std::deque<std::unique_ptr<bodylist>> cmds_;
        std::map<std::string, std::shared_ptr<file>> files_;
    private:
        stopwatch timer_;
        download_state_machine stm_;
        bitflag<ui::task::flags> errors_;
    private:
        bool overwrite_;
        bool discard_text_;        
    };

} // newsflash

