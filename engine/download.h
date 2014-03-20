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

#include <memory>
#include <vector>
#include <string>
#include "task.h"
#include "decoder.h"
#include "bigfile.h"
#include "stopwatch.h"
#include "encoding.h"

namespace newsflash
{
    // extract encoded content from the buffers
    class download : public task
    {
    public:
        download(std::string path, std::string name);

        // set overwrite flag to true/false.
        // overwrite defaults to true.
        void overwrite(bool val);

        // set keeptext flag to true/false
        // keeptext defaults to false.
        void keeptext(bool val);

        // task implementation
        void prepare() override;
        void receive(buffer&& buff, std::size_t id) override;
        void cancel() override;
        void flush() override;
        void finalize() override;

    private:
        struct content {
            std::size_t size;            
            std::string name;
            std::unique_ptr<decoder> codec;
            std::vector<std::string> errors;
            bigfile file;
            encoding enc;
        };

        content* find_content(encoding enc);
        content* bind_content(content&& cont);

    private:
        void on_info(const decoder::info& info, content& cont);
        void on_write(const void* data, std::size_t size, std::size_t offset, bool has_offset, content& cont);
        void on_error(decoder::error error, const std::string& what, content& cont);

    private:
        std::vector<content> contents_;
        std::string path_;
        std::string name_;
        bool overwrite_;
        bool keeptext_;
    };

} //  newsflash