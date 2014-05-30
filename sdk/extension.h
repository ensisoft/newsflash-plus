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

#if defined(LINUX_OS)
#  define EXTENSION_API extern "C" 
#elif defined(WINDOWS_OS)
#  ifdef EXTENSION_IMPL
#    define EXTENSION_API extern "C" __declspec(dllexport)
#  else
#    define EXTENSION_API extern "C" __declspec(dllimport)
#  endif
#endif

#include <newsflash/warnpush.h>
#  include <QtGlobal>     // for quint64
#  include <QString>
#include <newsflash/warnpop.h>
#include "extfwd.h"
#include "filetype.h"

class QMenu;

namespace sdk
{
    enum {
        // this version number covers the whole extension system, 
        // covering all APIs.(host/extension/gui/settings) Any change in the change in the APIs
        // bumps this version number up. While this means that the whole world
        // needs to be rebuilt it also reduces compatibility head aches.
        EXTENSION_API_VERSION = 13
    };

    struct download 
    {
        quint64  serverid;       // id of the service where this is coming from
        quint64  size;           // file size in bytes
        QString  name;           // human readable download name 
        QString  file;           // path to the file on the file system 
        bool     damaged;        // true if file was damaged
        bool     batched;        // true if part of a larger download batch
        filetype type;
    };

    class extension
    {
    public:
        virtual ~extension() = default;

        // Register commands that go to "open" menu in the host application.
        virtual void register_open_commands(QMenu& menu) {}

        // create a new GUI component. If the extension doesn't support
        // GUIs then this function can return NULL. The ownership of the GUI
        // remains with the extension.
        virtual uicomponent* provide_gui() { return NULL; }

        // Create a new settings GUI component. If the extension doesn't support
        // settings then this function can return NULL. The ownership of the created
        // GUI  is transferred to the host application.
        virtual uisettings* create_settings_gui() { return NULL; }

        // Try to process the given file. If file is supported and processed
        // the extension should return true, otherwise false.
        virtual bool accept_file(const QString& filepath) { return false; }

        // Notification functions when something interesting happens 
        // in the application.

        // Host application settings have changed
        virtual void settings_changed() {}

        // Ntofication function when configured servers/newsgroups have been changed.
        virtual void servers_changed() {}

        // Host application has encountered an error with a system specific error code.
        // What is a short english description of the error. 
        // Resource identifies the resource to which the error applies, i.e. network URL
        // or file path.
        // error is the platform specific error code.
        virtual void system_error(const QString& what, const QString& resource, unsigned int error) {}

        // Listing file was completed for the specified server
        virtual void listing_complete(const QString& server, const QString& listing_file) {}
        
        // Newsgroup update was completed for the specified server/group. Folder
        // is the location of the datafiles
        virtual void update_complete(const QString& server, const QString& group, const QString& folder) {}
        
        // Downloading a file was completed.
        // Server specifies the news server, name is the human readable name/description of the downloaded file.
        // File is the absolute platform specific path to the file. 
        virtual void download_complete(const QString& server, const QString& name, const QString& file) {}

        // Downloading a file was completed.
        virtual void download_complete(const download& file) {}
        
        // Downloading failed because the requested article was no longer
        // available on the server.
        // server specifies the news servers. name is the human readable name/description
        // of the download that was no longer available.
        virtual void download_unavailable(const QString& server, const QString& name) {}

        // A batched download was completed.
        // Server specifies the news server. Folder is the absolute platforms specific path
        // to the folder that contains the downloaded files.
        virtual void batch_complete(const QString& server, const QString& folder) {}
        
        // Prepare for shutdown. Note that after this it is possible
        // that the extension is not shutdown but operation continues.
        virtual void prepare_shutdown() {}

        // Shutdown this extension for good
        virtual void shutdown() {}
    };

// this is the mandatory entry point/factory function that every extension
// library needs to implement. 
// The only purpose for this function is to create a new extension* object
typedef extension* (*extension_factory)(newsflash*);

// This function should return the extension system API version the extension
// sdkension was compiled against.
// If the version number is different from the version number expected by the
// host application the extension will not be loaded.
typedef int (*version_function)();

// this function should return the current extension version
// with major, minor values
typedef void (*lib_version_function)(int*, int*);

} // sdk

  // factory function
EXTENSION_API sdk::extension* create_extension(sdk::newsflash*);

  // sdkension API version function
EXTENSION_API int extension_api_version();

  // this is added later, might not be implemented
EXTENSION_API void extension_lib_version(int* major, int* minor);




