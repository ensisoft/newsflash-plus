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
#  define PLUGIN_API extern "C" 
#elif defined(WINDOWS_OS)
#  ifdef PLUGIN_IMPL
#    define PLUGIN_API extern "C" __declspec(dllexport)
#  else
#    define PLUGIN_API extern "C" __declspec(dllimport)
#  endif
#endif

#include <newsflash/warnpush.h>
#  include <QtGlobal>
#  include <QList>
#  include <QString>
#include <newsflash/warnpop.h>
#include "extfwd.h"
#include "mediatype.h"
#include "release.h"
#include "types.h"

namespace sdk
{
    enum { 
        PLUGIN_API_VERSION = 1 
    };

    class newsflash;

    class plugin 
    {
    public:
        // features supported by the plugin
        enum features {
            has_rss    = 0x1,  // can provide RSS stream
            has_search = 0x2,  // can provide searching functionality
            has_info   = 0x4   // can provide ???
        };

        virtual ~plugin() = default;
        
        // Create a new settings GUI component. If the extension doesn't support
        // settings then this function can return NULL. The ownership of the created
        // GUI  is transferred to the host application.
        virtual uisettings* create_settings_gui() { return nullptr; }

        // Get request objects to receive RSS feeds in the specified category.
        // The ownership of the newed request objects are transferred to the caller.
        virtual QList<request*> get_rss(mediatype feed) { return QList<request*>(); }
        
        // Get a request object to get a NZB for the given RSS item
        // specified by the given ID.
        // The ownership of the request object is transferred to the caller
        virtual request* get_nzb(const QString& id, const QString& url) { return NULL; }

        // Get a request object to get item information for the given RSS item
        // specified by the given ID.
        // The ownership of the request object is transferred to the caller
        virtual request* get_information(const QString& id, const QString& url) { return NULL; }
        
        // get plugin name
        virtual QString name() const = 0;
        
        // get the host name that the plugin connects to
        virtual QString host() const = 0;

        // get supported features flag
        virtual bitflag_t features() const = 0;
        
        virtual void prepare_shutdown() { }
        
        virtual void shutdown() {}
        
        virtual bool is_enabled() const { return true; }
    protected:
    private:
    };
    
    typedef plugin* (*plugin_factory)(newsflash*);
    
} // sdk

  // factory function, create a plugin object
  PLUGIN_API sdk::plugin* create_plugin(sdk::newsflash*);
  
  // get the api version implemented by the plugin objects
  PLUGIN_API int plugin_api_version();
  
  // get the library version
  PLUGIN_API void plugin_lib_version(int* major, int* minor);
