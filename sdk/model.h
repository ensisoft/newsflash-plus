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
#  include <QVariant>
#  include <QString>
#include <newsflash/warnpop.h>

#include "plugin.h"

class QAbstractItemModel;

namespace sdk
{
    class datastore;
    class hostapp;
    class request;

    // model implementations are application plugins 
    // that represent application data and provide actions
    // for the GUI to invoke upon that data.
    // typically the GUI obtains a reference to the model
    // and then calls model methods directly.
    class model
    {
    public:
        enum {
            version = 1
        };

        virtual ~model() = default;

        // clear all the data in the model
        virtual void clear() {};

        // returns true if model has no data.
        virtual bool is_empty() const { return true; }

        // get a Qt view compatible model object
        // for quickly showing the data in a Qt view widget
        virtual QAbstractItemModel* view() { return nullptr; }

        // save model state to the datastore.
        virtual void save(datastore& store) const {}

        // load model state from the datastore.
        virtual void load(const datastore& store) {}

        virtual void complete(request* request) {};

        virtual bool shutdown() { return true; }

        virtual QString name() const = 0;

    protected:
    private:

    };

    typedef model* (*fp_model_create)(sdk::hostapp*, const char*, int);

} // sdk

  // factory function, create a plugin object. 
  // this function may not throw an exception but should return nullptr on error
  PLUGIN_API sdk::model* create_model(sdk::hostapp*, const char* klazz, int version);
  