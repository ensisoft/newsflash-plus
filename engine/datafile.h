// Copyright (c) 2010-2015 Sami Väisänen, Ensisoft
//
// http://www.ensisoft.com
//
// This software is copyrighted software. Unauthorized hacking, cracking, distribution
// and general assing around is prohibited.
// Redistribution and use in source and binary forms, with or without modification,
// without permission are prohibited.
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#include "newsflash/config.h"

#include <string>
#include <mutex>
#include <vector>
#include <functional>

#include "assert.h"
#include "bigfile.h"
#include "action.h"
#include "filesys.h"

namespace newsflash
{
    class DataFile
    {
    public:
        DataFile(const std::string& filepath,
                 const std::string& filename,
                 const std::string& dataname,
                 bool is_binary)
        {
            impl_ = std::make_shared<Impl>(filepath, filename,  dataname, is_binary);
        }
        DataFile(const std::string& path,
                 const std::string& binaryname,
                 std::size_t size,
                 bool is_binary,
                 bool overwrite)
        {
            impl_ = std::make_shared<Impl>(path, binaryname, size, is_binary, overwrite);
        }
        void Close()
        {
            impl_->CloseWhenPossible();
        }

        struct WriteComplete {
            std::uint64_t size = 0;
            std::size_t offset = 0;
        };

        using OnWriteDone = std::function<void (const WriteComplete&)>;

        std::unique_ptr<action> Write(std::size_t offset, const std::vector<char>& data,
            const OnWriteDone& callback = OnWriteDone())
        {
            // note that there's a little hack here and the offset is offset by +1
            // so that we're using 0 for indicating that offset is not being used at all.
            auto write = std::make_unique<WriteOp>(offset, data, impl_);
            write->set_affinity(action::affinity::single_thread);
            write->set_callback(callback);
            impl_->AddPendingWrite();
            return write;
        }

        void DiscardOnClose()
        { impl_->DiscardOnClose(); }
        std::uint64_t GetFileSize() const
        { return impl_->GetFileSize(); }
        const std::string& GetFileName() const
        { return impl_->GetFileName(); }
        const std::string& GetFilePath() const
        { return impl_->GetFilePath(); }
        const std::string& GetBinaryName() const
        { return impl_->GetBinaryName(); }
        bool IsBinary() const
        { return impl_->IsBinary(); }
        bool IsOpen() const
        { return impl_->IsOpen(); }

    private:
        class Impl
        {
        public:
            Impl(const std::string& filepath,
                 const std::string& filename,
                 const std::string& dataname,
                 bool is_binary)
            : filepath_(filepath)
            , filename_(filename)
            , dataname_(dataname)
            , binary_(is_binary)
            {
                file_.open(fs::joinpath(filepath_, filename_));
            }
            Impl(const std::string& path,
                 const std::string& binaryname,
                 std::size_t size,
                 bool is_binary,
                 bool overwrite)
            : binary_(is_binary)
            {
                std::string file;
                std::string name;
                std::string accepted_name = fs::remove_illegal_filename_chars(binaryname);
                // try to open the file at the given location under different
                // filenames unless ovewrite flag is set.
                for (int i=0; i<10; ++i)
                {
                    name = fs::filename(i, accepted_name);
                    file = fs::joinpath(path, name);
                    if (!overwrite && bigfile::exists(file))
                        continue;

                    // todo: there's a race condition between the call to exists and the open below.
                    file_.open(file, bigfile::o_create | bigfile::o_truncate);
                    if (size)
                    {
                        const auto error = bigfile::resize(file, size);
                        if (error)
                            throw std::system_error(error, "error resizing file: " + file);
                    }
                    break;
                }
                if (!file_.is_open())
                    throw std::runtime_error("unable to create file at: " + path);

                filepath_ = path;
                filename_ = name;
                dataname_ = binaryname;
            }
           ~Impl()
            {
                Close();
            }
            void AddPendingWrite()
            {
                std::lock_guard<std::mutex> lock(mutex_);
                ++num_writes_;
            }

            void CloseWhenPossible()
            {
                std::lock_guard<std::mutex> lock(mutex_);

                if (!file_.is_open())
                    return;

                if (num_writes_)
                {
                    close_on_last_write_ = true;
                    return;
                }
                Close();

            }
            void Write(std::size_t offset, const std::vector<char>& data)
            {
                std::lock_guard<std::mutex> lock(mutex_);

                if (!discard_)
                {
                    // see the comment in the constructor about the offset value
                    if (offset)
                        file_.seek(offset - 1);
                    file_.write(&data[0], data.size());
                }
                ASSERT(num_writes_);

                if (--num_writes_ == 0 && close_on_last_write_)
                {
                    Close();
                }
            }

            void Close()
            {
                if (!file_.is_open())
                    return;

                finalsize_ = file_.size();
                file_.close();
                if (discard_)
                    bigfile::erase(fs::joinpath(filepath_, filename_));
            }

            void DiscardOnClose()
            {
                std::lock_guard<std::mutex> lock(mutex_);
                discard_ = true;
            }

            std::uint64_t GetFileSize() const
            { return finalsize_; }

            const std::string& GetFileName() const
            { return filename_; }

            const std::string& GetFilePath() const
            { return filepath_; }

            const std::string& GetBinaryName() const
            { return dataname_; }

            bool IsBinary() const
            { return binary_; }

            bool IsOpen() const
            { return file_.is_open(); }
        private:
            std::mutex mutex_;
            std::size_t num_writes_ = 0;
            std::string filepath_;
            std::string filename_;
            std::string dataname_;
            std::uint64_t finalsize_ = 0;
            bool binary_ = false;
            bool discard_ = false;
            bool close_on_last_write_ = false;
            bigfile file_;
        };

        class WriteOp : public action
        {
        public:
            // note that there's a little hack here and the offset is offset by +1
            // so that we're using 0 for indicating that offset is not being used at all.
            WriteOp(std::size_t offset,
                const std::vector<char>& data, std::shared_ptr<Impl> impl)
            : impl_(impl)
            , data_(data)
            , offset_(offset)
            {}

            virtual void xperform() override
            {
                impl_->Write(offset_, data_);
            }
            virtual std::string describe() const override
            { return "DataFile::WriteOp"; }

            virtual void run_completion_callbacks() override
            {
                if (callback_)
                {
                    WriteComplete completion;
                    completion.size = data_.size();
                    completion.offset = offset_;
                    callback_(completion);
                }
            }
            void set_callback(const OnWriteDone& callback)
            { callback_ = callback; }

        private:
            std::shared_ptr<Impl> impl_;
            std::vector<char> data_;
            std::size_t offset_ = 0;
            OnWriteDone callback_;
        };
    private:
        std::shared_ptr<Impl> impl_;
    };

} // newsflash
