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

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QJsonDocument>
#  include <QJsonArray>
#  include <QJsonObject>
#  include <QVariant>
#  include <QScopedPointer>
#  include <QIODevice>
#  include <QDebug>
#include <newsflash/warnpop.h>
#include <stdexcept>
#include <sstream>
#include "settings.h"
#include "format.h"
#include "debug.h"

namespace app
{

Settings::Settings()
{
    DEBUG("Settings created");
}

Settings::~Settings()
{
    DEBUG("Settings deleted");
}


void Settings::load(QIODevice& io, Settings::format format)
{
    Q_ASSERT(io.isOpen());

    if (format == Settings::format::json)
    {
        QJsonParseError error;
        QJsonDocument doc(QJsonDocument::fromJson(io.readAll(), &error));
        if (doc.isNull())
        {
            const auto& line = error.offset;
            const auto& what = error.errorString();
            throw std::runtime_error(narrow(toString("JSON parse failed %1 at line %2", what, line)));
        }

        QVariantMap db;

        const QJsonObject& root = doc.object();
        for (auto it = root.begin(); it != root.end(); ++it)
        {
            const auto& context = it.key();
            const auto& values  = it.value().toObject().toVariantMap();
            db[context] = values;
        }
        m_values = db;
    }
}

void Settings::save(QIODevice& io, Settings::format format) const
{
    Q_ASSERT(io.isOpen());

    if (format == Settings::format::json)
    {
        QJsonObject root;
        for (auto it = m_values.begin(); it != m_values.end(); ++it)
        {
            const QVariantMap& values = it.value().toMap();
            root[it.key()] = QJsonObject::fromVariantMap(values);
        }
        QJsonDocument docu(root);
        io.write(docu.toJson());
    }
}

QFile::FileError Settings::load(const QString& file, Settings::format format)
{
    QFile io(file);
    if (!io.open(QIODevice::ReadOnly))
        return io.error();

    load(io, format);
    return QFile::NoError;
}

QFile::FileError Settings::save(const QString& file, Settings::format format)
{
    QFile io(file);
    if (!io.open(QIODevice::WriteOnly))
        return io.error();

    save(io, format);
    return QFile::NoError;
}

void Settings::clear()
{
    m_values.clear();
}

bool Settings::contains(const char* context, const char* name) const
{
    const auto& value = get(context, name);
    if (value.isNull())
        return false;

    return true;
}

QVariant Settings::get(const QString& key, const QString& attr, const QVariant& defval) const
{
    const QVariantMap& map = m_values[key].toMap();
    if (map.isEmpty())
        return defval;

    const QVariant& value = map[attr];
    return value;
}

void Settings::set(const QString& key, const QString& attr, const QVariant& value)
{
    QVariantMap map = m_values[key].toMap();
    map[attr] = value;
    m_values[key] = map;
}

void Settings::del(const QString& key)
{
    m_values.remove(key);
}

void Settings::merge(const Settings& other)
{
    const auto& values = other.m_values;
    for (auto it = values.begin(); it != values.end(); ++it)
    {
        const QString&  key = it.key();
        const QVariant& tmp = it.value();

        QVariantMap merged  = m_values[key].toMap();

        const QVariantMap& map = tmp.toMap();
        for (auto it = map.begin(); it != map.end(); ++it)
        {
            const QString&  attr = it.key();
            const QVariant& val  = it.value();
            if (merged.contains(attr)) {
                throw std::runtime_error(narrow(
                    toString("trying to merge a duplicate value (%1/%2)", key, attr)));
            }
            merged[attr] = val;
        }
        m_values[key] = merged;
    }
}

} // app

