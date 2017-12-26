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

#include <functional>
#include <memory>
#include <vector>
#include <string>

#include "action.h"
#include "bitflag.h"

namespace newsflash
{
    class CmdList;

    // tasks implement some nntp data based activity in the engine, for
    // example extracting binary content from article data.
    class Task
    {
    public:
        struct Settings {
            // if set to true engine will overwrite files
            // that already exist in the filesystem.
            // if set to false, new files will be renamed
            // to avoid collisions.
            bool overwrite_existing_files = false;

            // set to true to discard any textual data
            // that is downloaded as a part of downloading
            // some binary content.
            // set to false to have the text data stored to the disk.
            bool discard_text_content = true;
        };

        enum class Error {
            CrcMismatch,
            SizeMismatch
        };

        virtual ~Task() = default;

        // Create a command list object with details about
        // what data to read from the remote host.
        virtual std::shared_ptr<CmdList> CreateCommands() = 0;

        // cancel the task. if the task is not complete then this has the effect
        // of canceling all the work that has been done, for example removing
        // any files created on the filesystem etc. if task is complete then
        // simply the non-persistent data is cleaned away.
        // after this call returns the object can be deleted.
        virtual void Cancel() {}

        // commit our completed task.
        virtual void Commit() {}

        // complete the action. create actions (if any) and store
        // them in the next list
        virtual void Complete(action& act,
            std::vector<std::unique_ptr<action>>& next) {}

        // complete the cmdlist. create actions (if any) and store
        // them in the next list
        virtual void Complete(CmdList& cmd,
            std::vector<std::unique_ptr<action>>& next) {}

        // update task settings
        virtual void Configure(const Settings& settings) {}

        virtual bool HasCommands() const = 0;

        // return the current progress percentage (0.0f - 100.0f)
        virtual float GetProgress() const
        { return 0.0f; }

        // Lock the task for the calling thread.
        // Locking allows the calling thread to acquire a consistent
        // view of the current task state by preventing any other
        // thread from making modifications to the task object.
        // The calling thread must call Unlock when done.
        virtual void Lock() {}

        // Unlock the task lock. see Lock
        virtual void Unlock() {}

        // get any (content) errors encountered by the task so far.
        virtual bitflag<Error> GetErrors() const
        {
            return bitflag<Error>();
        }

        // returns true if the task can serialize (pack)
        // and load (load) it's state.
        virtual bool CanSerialize() const
        { return false; }

        // pack the task state into byte buffer
        virtual void Pack(std::string* data) const
        {}

        // unpack the task state from the byte buffer
        virtual void Load(const std::string& data)
        {}

    protected:
    private:
    };

} // newsflash