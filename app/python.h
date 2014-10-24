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

#if defined(NEWSFLASH_ENABLE_PYTHON)

#include <newsflash/warnpush.h>
#  include <QAbstractTableModel>
#  include <QObject>
#include <newsflash/warnpop.h>
#include <memory>
#include <vector>

#include <Python.h>

namespace app
{
    class settings;

    class python : public QObject
    {
        Q_OBJECT

    public:
        python();
       ~python();

        void loadstate(settings& s);
        void savestate(settings& s);

    private:
        virtual bool eventFilter(QObject* object, QEvent* event) override;

    private:
        //PyThreadState* thread_state_;
        //PyInterpreterState* interp_state_;
        //std::vector<std::unique_ptr<script>> scripts_;
    };
} // app

#endif // ENABLE_PYTHON