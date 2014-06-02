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

#include <newsflash/warnpush.h>
#  include <QVariant>
#  include <QMap>
#include <newsflash/warnpop.h>

class QIODevice;

namespace app
{
    // store arbitrary values/objects with a context/name key.
    // values are keyed by context-name pair so that for example
    // "some-context", "value" and "other-context", "value" point
    // to different actual value objects.
    class settings
    {
    public:
        enum class format {
            json
        };

        settings();
       ~settings();

        // load the store from the given stream.
        // io must be already opened.
        void load(QIODevice& io, settings::format format = format::json);

        // save the store to a stream.
        // io must be already opened.
        void save(QIODevice& io, settings::format format = format::json) const;

        // clear all values
        void clear();

        // returns true if a value identifief by context/name key
        // exists in the settings. otherwise returns false.
        bool contains(const char* context, const char* name) const;

        // set the value for the named attribute under the specified key
        void set(const QString& key, const QString& attr, const QVariant& value);

        // get the value under the named attribute in the specified key
        QVariant get(const QString& key, const QString& attr, const QVariant& defval = QVariant()) const;

        // convenience accessor
        template<typename T>
        T get(const QString& key, const QString& attr, const T& defval) const
        {
            const auto& var = get(key, attr);
            if (var.isValid())
                return qvariant_cast<T>(var);
            return qvariant_cast<T>(defval);
        }

        QString get(const QString& key, const QString& attr, const char* str) const
        {
            const auto& var = get(key, attr);
            if (var.isValid())
                return qvariant_cast<QString>(var);
            return QString(str);
        }

    private:
        QVariantMap values_;
    };

} // app
