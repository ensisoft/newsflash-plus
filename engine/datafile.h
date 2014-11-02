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

#include <newsflash/config.h>
#include <string>
#include <atomic>
#include <vector>
#include <cassert>
#include "bigfile.h"
#include "action.h"
#include "filesys.h"

namespace newsflash
{
    class datafile 
    {
    public:
        class write : public action
        {
        public:
            // note that there's a little hack here and the offset is offset by +1
            // so that we're using 0 for indicating that offset is not being used at all.
            write(std::size_t offset, 
                std::vector<char> data, std::shared_ptr<datafile> file) : offset_(offset),
                data_(std::move(data)), file_(file)
            {
            #ifdef NEWSFLASH_DEBUG
                file_->num_writes_++;
            #endif
                set_affinity(affinity::single_thread);
            }

           ~write()
            {
            #ifdef NEWSFLASH_DEBUG
                file_->num_writes_--;
            #endif
            }

            virtual void xperform() override
            {
                if (file_->discard_)
                    return;

                // see the comment in the constructor about the offset value
                if (offset_)
                    file_->big_.seek(offset_-1);

                file_->big_.write(&data_[0], data_.size());
            }
            std::size_t get_write_size() const 
            { return data_.size(); }

        private:
            std::size_t offset_;
            std::vector<char> data_;
            std::shared_ptr<datafile> file_;
        };

        datafile(std::string path, std::string binaryname, std::size_t size, bool bin, bool overwrite) : discard_(false), binary_(bin)
        {
            std::string file;
            std::string name = fs::remove_illegal_filename_chars(binaryname);
            // try to open the file at the given location under different
            // filenames unless ovewrite flag is set.
            for (int i=0; i<10; ++i)
            {
                name = fs::filename(i, name);
                file = fs::joinpath(path, name);
                if (!overwrite && bigfile::exists(file))
                    continue;

                const auto error = big_.create(file);
                if (error)
                    throw std::system_error(error, "error creating file: " + file);
                if (size)
                {
                    const auto error = bigfile::resize(file, size);
                    if (error)
                        throw std::system_error(error, "error resizing file: " + file);
                }
                break;
            }
            if (!big_.is_open())
                throw std::runtime_error("unable to create file at: " + path);

            path_ = path;
            name_ = name;
            binary_name_ = binaryname;
        #ifdef NEWSFLASH_DEBUG
            num_writes_ = 0;
        #endif
        }

       ~datafile()
        {
            close();
        }

        void close()
        {
            if (!big_.is_open())
                return;

        #ifdef NEWSFLASH_DEBUG
            assert(num_writes_ == 0);
        #endif

            big_.close();
            if (discard_)
                bigfile::erase(big_.name());            
        }

        void discard_on_close()
        { discard_ = true; }

        std::uint64_t size() const 
        { return big_.size(); }

        const std::string& name() const
        { return name_; }

        const std::string& path() const 
        { return path_; }

        const std::string& binary_name() const 
        { return binary_name_; }

        bool is_binary() const 
        { return binary_; }

        bool is_open() const
        { return big_.is_open(); }

    private:
        friend class write;

        bigfile big_;
        std::atomic<bool> discard_;
        std::string path_;
        std::string name_;
        std::string binary_name_;
        bool binary_;
    #ifdef NEWSFLASH_DEBUG
        std::atomic<std::size_t> num_writes_;
    #endif
    };

} // newsflash