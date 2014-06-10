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

#include <newsflash/warnpush.h>
#  include <QString>
#include <newsflash/warnpop.h>

namespace sdk
{
    // applications home directory in user home
    class home
    {
    public:
        // initialize once. 
        // folder is name for our application specific folder
        // in the user's real home. for example /home/roger/ on 
        // a linux system and "c:\Documents and Settings\roger\"
        // on a windows system, so we get 
        // "home/roger/folder" and "c:\documents and settings\roger\folder".
        static 
        void init(const QString& folder);

        // get absolute path to the applications home directory
        static 
        QString path();

        // get the path to a file in the home directory 
        // in system specific path notation.
        static 
        QString file(const QString& name);

    private:
        static QString pathstr;
    };

} // sdk
