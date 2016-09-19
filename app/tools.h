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

#include "newsflash/warnpush.h"
#  include <QtGui/QIcon>
#  include <QString>
#  include <QObject>
#include "newsflash/warnpop.h"

#include <vector>
#include <memory>

#include "engine/bitflag.h"
#include "filetype.h"
#include "settings.h"

namespace app
{
    struct FileInfo;
    struct FilePackInfo;
    struct Archive;

    // tools is a collection of 3rd party application's that are configured
    // in newsflash and associated with particular file types for quick launch.
    // for example the user can associate "eog" as an image viewer application
    // and tag it for "image" media types and then launch eog on a particular
    // image file downloaded. 
    class tools : public QObject
    {
        Q_OBJECT

    public:
        using bitflag = newsflash::bitflag<FileType>;

        // tool/application details.
        class tool 
        {
        public:
            tool();
           ~tool();

            // get tool name
            const QString& name() const 
            { return name_; }

            // get tool native binary in the filesystem
            // this should be the complete path to the binary.
            const QString& binary() const 
            { return binary_; }

            // get arguments to be passed to the binary when executed.
            const QString& arguments() const 
            { return arguments_; }

            // get currently assigned shortcut.            
            const QString& shortcut() const 
            { return shortcut_; }

            // get the file types that this tool is applicable for.
            const bitflag types() const 
            { return types_; }

            // get native binary icon (if possible).
            const QIcon& icon() const;            

            // get the unique tool id.
            quint32 guid() const { return guid_; }

            // set tool human readable name. 
            void setName(const QString& name) 
            { name_ = name; }

            // set the tool binary. this should be the absolute
            // path to the application the machine's file system.
            void setBinary(const QString& binary) 
            { binary_ = binary; }

            // set the arguments to be passed to the tool when launching.
            // argument replacements:
            // ${file} will be replaced by the File parameter
            void setArguments(const QString& args) 
            { arguments_ = args; }

            // set application shortcut. using this depends on the 
            // context where the tool is used. i.e. if the actual
            // shortcut is registered in the system or not.
            void setShortcut(const QString& shortcut) 
            { shortcut_ = shortcut; }

            // set the applicable file types.
            void setTypes(bitflag f) 
            { types_ = f; };

            bool isValid() const 
            { return !binary_.isEmpty(); }

            bool startNewInstance(const QString& file) const;

        private:
            quint32 guid_;
            QString name_;
            QString binary_;
            QString arguments_;
            QString shortcut_;
            bitflag types_;
            mutable QIcon icon_;            
        };  

        // load tools collection.
        void loadState(Settings& set);

        // save tools collection.
        void saveState(Settings& set);

        // get a list of all the tools
        std::vector<const tool*> get_tools() const;

        // get a list of tools tagged with the matching types.
        std::vector<const tool*> get_tools(bitflag types) const;

        std::vector<tool> get_tools_copy() const 
        { return tools_; }

        void set_tools_copy(std::vector<tool> new_tools);

        tool* get_tool(quint32 guid);

        void add_tool(tools::tool tool);

        void del_tool(std::size_t row);

        std::size_t num_tools() const 
        { return tools_.size(); }

        bool empty() const 
        { return tools_.empty(); }

        bool has_tool(const QString& executable);

    signals:
        void toolsUpdated();

    private:
        std::vector<tool> tools_;     
    };
    
    extern tools* g_tools;

} // app