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

#include <newsflash/warnpush.h>
#  include <QString>
#  include <QStringList>
#include <newsflash/warnpop.h>

namespace sdk
{
    class model;

    // main window interface for widgets to communicate
    // requests back to the host window. 
    class window
    {
    public:
        // request for a new model object with the specified
        // class type to be created. if the model is succesfully
        // created a non-null pointer is returned. 
        virtual sdk::model* create_model(const char* klazz) = 0;

        // show/navigate to a specific widget in the user interface.
        virtual void show_widget(const QString& name) = 0;

        // open and show the settings dialog with the specified
        // settings interface open.
        virtual void show_setting(const QString& name) = 0;

        // select a folder for downloading content.
        // if the selection is cancelled a empty() string is returned.
        virtual QString select_download_folder() = 0;

        // select a nzb file for opening.
        //virtual QString open_nzb_file() = 0;

        // 
        //virtual QString save_nzb_file() = 0;

        virtual void recents(QStringList& paths) const = 0;
    protected:
        ~window() = default;
    };
} // sdk