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

#define LOGTAG "app"

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <boost/version.hpp>
#  include <QCoreApplication>
#  include <QStringList>
#  include <QFile>
#  include <QDir>
#  include <QEvent>
#include <newsflash/warnpop.h>

#include <newsflash/engine/engine.h>
#include <newsflash/engine/account.h>
#include <newsflash/engine/listener.h>
#include <newsflash/sdk/format.h>
#include <newsflash/sdk/debug.h>
#include <newsflash/sdk/eventlog.h>
#include <newsflash/sdk/home.h>
#include <vector>
#include "mainapp.h"
#include "accounts.h"
#include "groups.h"

using sdk::str;
using sdk::str_a;
using sdk::utf8;
using sdk::narrow;
using sdk::utf8;
using sdk::latin;

namespace {
    // we post this event object to the application's event queue
    // when a thread in the engine calls async_notify.
    // mainapp then installs itself as a global event filter
    // and uses the filter function to pick up the notification event
    // to drive the engine's event pump
    class async_notify_event : public QEvent 
    {
    public:
        async_notify_event() : QEvent(identity())
        {}

        static 
        QEvent::Type identity() 
        {
            static auto id = QEvent::registerEventType();
            return (QEvent::Type)id;
        }
    private:
    };

} // namespace

namespace app
{

mainapp::mainapp(QCoreApplication& app) : app_(app)
{
    const auto home = QDir::homePath();
    settings_.data_path            = QDir::toNativeSeparators(home + "/Newsflash/Data");
    settings_.logs_path            = QDir::toNativeSeparators(home + "/Newsflash/Logs");
    settings_.downloads_path       = QDir::toNativeSeparators(home + "/Newsflash/Downloads");
    settings_.enable_throttle      = false;
    settings_.discard_text_content = false;
    settings_.overwrite_existing   = false;
    settings_.remove_complete      = false;
    settings_.prefer_secure        = true;
    settings_.throttle             = 0;

    //models_.push_back(std::unique_ptr<sdk::model>(new accounts));
    //models_.push_back(std::unique_ptr<sdk::model>(new groups));

    const auto file  = sdk::home::file("engine.json");
    if (QFile::exists(file))
    {
        const auto error = data_.load(file);
        if (error != QFile::NoError)
        {
            ERROR(str("Failed to read settings _1, _2", error, file));
        }

        for (auto& model : models_)
        {
            model->load(data_);
        }        
        sdk::eventlog::get().load(data_);
        groups_.load(data_);
        accounts_.load(data_);

        load_settings();
    }

    engine_.reset(new newsflash::engine(*this, utf8(settings_.logs_path)));

    app.installEventFilter(this);

    DEBUG("Application created");    
}

mainapp::~mainapp()
{
    app_.removeEventFilter(this);

    DEBUG("Application deleted");
}

sdk::model& mainapp::get_model(const QString& name)
{
    if (name == sdk::eventlog::get().name())
        return sdk::eventlog::get();
    else if (name == accounts_.name())
        return accounts_;
    else if (name == groups_.name())
        return groups_;

    auto it = std::find_if(std::begin(models_), std::end(models_),
        [&](const std::unique_ptr<sdk::model>& model) {
            return model->name() == name;
        });
    if (it == std::end(models_))
        throw std::runtime_error("no such model");

    auto& ptr = *it;

    return *ptr.get();
}

bool mainapp::savestate()
{
    data_.clear();

    sdk::eventlog::get().save(data_);

    groups_.save(data_);
    accounts_.save(data_);

    for (auto& model : models_)
    {
        model->save(data_);
    }

    save_settings();

    const auto file  = sdk::home::file("engine.json");
    const auto error = data_.save(file);
    if (error != QFile::NoError)
    {
        ERROR(str("Error saving settings _1 _2", error, file));
        return false;
    }
    
    // todo: save engine state

    return true;
}

void mainapp::shutdown()
{
    engine_->stop();
}

void mainapp::handle(const newsflash::error& error)
{}


void mainapp::acknowledge(const newsflash::file& file)
{}


void mainapp::async_notify()
{
    const auto& tid = std::this_thread::get_id();
    std::stringstream ss;
    std::string s;
    ss << tid;
    ss >> s;

    DEBUG(str("Async notify by thread: _1", s));

    // postEvent is a thread safe function  so we're all se here.
    QCoreApplication::postEvent(this, new async_notify_event());
}

bool mainapp::eventFilter(QObject* object, QEvent* event)
{
    if (object != this)
        return false;

    if (event->type() != async_notify_event::identity())
        return false;

    engine_->pump();
    return true;

}

void mainapp::save_settings()
{
#define SAVE(x) \
    data_.set("settings", #x, settings_.x)

    SAVE(logs_path);
    SAVE(data_path);
    SAVE(downloads_path);
    SAVE(enable_throttle);
    SAVE(discard_text_content);
    SAVE(overwrite_existing);
    SAVE(remove_complete);
    SAVE(prefer_secure);
    SAVE(throttle);

#undef SAVE
}

void mainapp::load_settings()
{

#define LOAD(x) \
    settings_.x = data_.get("settings", #x, settings_.x)

    LOAD(logs_path);
    LOAD(data_path);
    LOAD(downloads_path);
    LOAD(enable_throttle);
    LOAD(discard_text_content);
    LOAD(overwrite_existing);
    LOAD(remove_complete);
    LOAD(prefer_secure);
    LOAD(throttle);

#undef LOAD

}


} // app
