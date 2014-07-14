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
#include <newsflash/warnpop.h>

#include <memory>

#include "mainmodel.h"
#include "nzbthread.h"

class QFile;

namespace app
{
    class nzbfile : public QObject, public mainmodel
    {
        Q_OBJECT

    public:
        nzbfile();
       ~nzbfile();

        virtual void clear() override;

        virtual QAbstractItemModel* view() override;

        // begin loading the NZB contents from the given file
        // returns true if file was succesfully opened and then subsequently
        // emits ready() once the file has been parsed.
        // otherwise returns false and no signal will arrive.
        bool load(const QString& file);

    signals:
        void ready();

    private slots:
        void parse_complete();

    private:
        struct item;
        class model;

    private:
        std::unique_ptr<nzbthread> thread_;
        std::unique_ptr<model> model_;
    };

} // app
