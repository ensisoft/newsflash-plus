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

#if defined(LINUX_OS)
#  define WIDGET_API extern "C" 
#elif defined(WINDOWS_OS)
#  ifdef WIDGET_IMPL
#    define WIDGET_API extern "C" __declspec(dllexport)
#  else
#    define WIDGET_API extern "C" __declspec(dllimport)
#  endif
#endif

#include <newsflash/warnpush.h>
#  include <QtGui/QWidget>
#  include <QString>
#include <newsflash/warnpop.h>

class QMenu;
class QToolBar;

namespace sdk
{
    class datastore;

    // widget objects extend the use visible interface.
    // a widget typically uses a model object and presents
    // the data within the model to the user and responds
    // to user interactions.
    class widget : public QWidget
    {
    public:
        struct info {
            // this is the URL to the help (file). If it specifies a filename
            // it is considered to be a help file in the application's help installation
            // folder. Otherwise an absolute path or URL can be specified.            
            QString helpurl;

            // Whether the component should be visible by default (on first launch).
            bool visible_by_default;            
        };

        virtual ~widget() = default;
        
        // Add the component specific menu actions to a menu inside the host application
        virtual void add_actions(QMenu& menu) {}
        
        // Add the component specific toolbar actions to a toolbar in the host application
        virtual void add_actions(QToolBar& bar) {}
        
        // This function is invoked when this ui component is getting activated (becomes visible)
        // in the host GUI.
        virtual void activate(QWidget* parent) {}
        
        // This function is invoked when this ui component is hidden in the host GUI.
        virtual void deactivate() {}

        // save widget state into datastore
        virtual void save(datastore& store) {}

        // load widget state from datastore.
        virtual void load(const datastore& store) {}

        // get information about the widget.
        virtual info information() const { return {"", false}; }
    private:
    };
    
} // sdk

