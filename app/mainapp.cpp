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
#  include <QtNetwork/QNetworkReply>
#  include <QtNetwork/QNetworkRequest>
#  include <QCoreApplication>
#  include <QStringList>
#  include <QFile>
#  include <QDir>
#  include <QEvent>
#  include <QLibrary>
#include <newsflash/warnpop.h>

#include <newsflash/engine/engine.h>
#include <newsflash/engine/account.h>
#include <newsflash/engine/listener.h>
#include <newsflash/sdk/format.h>
#include <newsflash/sdk/debug.h>
#include <newsflash/sdk/eventlog.h>
#include <newsflash/sdk/home.h>
#include <newsflash/sdk/dist.h>
#include <newsflash/sdk/request.h>
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

    net_submit_timer_  = 0;
    net_timeout_timer_ = 0;

    QObject::connect(&net_, SIGNAL(finished(QNetworkReply*)),
        this, SLOT(replyAvailable(QNetworkReply*)));
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

sdk::model* mainapp::create_model(const char* klazz)
{
    DEBUG(str("Create instance of _1", klazz));

#define FAIL(x) \
    if (1) { \
        ERROR(str("Failed to load plugin library _1, _2", lib, x)); \
        continue; \
    }

    const QDir dir(sdk::dist::path("plugins"));

    for (const auto& name : dir.entryList())
    {
        const auto file = dir.absoluteFilePath(name);
        if (!QLibrary::isLibrary(file))
            continue;

        QLibrary lib(file);

        DEBUG(str("Loading plugin _1", lib));

        if (!lib.load())
        {
            ERROR(lib.errorString());
            continue;
        }

        auto create = (sdk::fp_model_create)(lib.resolve("create_model"));
        if (create == nullptr)
            FAIL("no entry point found");

        std::unique_ptr<sdk::model> model(create(this, klazz, sdk::model::version));
        if (!model)
            continue;

        models_.push_back(std::move(model));

        INFO(str("Loaded _1", lib));

        return models_.back().get();
    }

#undef FAIL

    DEBUG(str("No such model _1", klazz));

    return nullptr;
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

void mainapp::submit(sdk::model* model, sdk::request* req)
{
    const auto submit_interval_millis = 1000;

    submits_.push_back({0, req, model, nullptr});
    if (submits_.size() == 1)
    {
        submit_first();
    }   
    else if (!net_submit_timer_)
    {
        startTimer(submit_interval_millis);
    }
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

void mainapp::replyAvailable(QNetworkReply* reply)
{
    QString err;

#define CASE(x) case QNetworkReply::x##Error : err = #x"Error"; break;

    switch (reply->error())
    {
        CASE(ConnectionRefused);
        CASE(RemoteHostClosed);
        CASE(HostNotFound);
        CASE(Timeout);
        CASE(OperationCanceled);
        CASE(SslHandshakeFailed);
        CASE(TemporaryNetworkFailure);
        CASE(ProxyConnectionRefused);
        CASE(ProxyConnectionClosed);
        CASE(ProxyNotFound);
        CASE(ProxyAuthenticationRequired)
        CASE(ProxyTimeout);
        CASE(ContentOperationNotPermitted);
        CASE(ContentNotFound);
        CASE(ContentReSend);
        CASE(AuthenticationRequired);
        CASE(ProtocolUnknown);
        CASE(ProtocolInvalidOperation);
        CASE(UnknownNetwork);
        CASE(UnknownProxy);
        CASE(UnknownContent);

        case QNetworkReply::ContentAccessDenied:
            err = "ContentAccessDenied";
            break;
        case QNetworkReply::ProtocolFailure: 
            err = "ProtocolFailure"; 
            break;
        case QNetworkReply::NoError:
            err = "Success";
            break;
    }

    DEBUG(str("replyAvailable _1, _2", reply->url(), err));

#undef CASE

    if (reply->error() != QNetworkReply::NoError)
    {
        ERROR(str("Network request _1 failed, _2", reply->url(), err));
    }
    
    auto it = std::find_if(std::begin(submits_), std::end(submits_),
        [=](const submission& sub) {
            return sub.reply == reply;
        });
    Q_ASSERT(it != std::end(submits_));

    auto& submit = *it;

    submit.handler->receive(*reply);
    submit.model->complete(submit.handler);

    reply->deleteLater();

    submits_.erase(it);
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

void mainapp::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == net_submit_timer_)
    {
        if (!submit_first())
        {
            killTimer(net_submit_timer_);
            net_submit_timer_ = 0;
        }
    }
    else if (event->timerId() == net_timeout_timer_)
    {
        const auto timeout_ticks = 30;

        std::for_each(std::begin(submits_), std::end(submits_),
            [](submission& m) {
                if (m.reply)
                    ++m.ticks;
            });

        std::list<submission> pending;
        std::list<submission> timeouts;

        // very carefully now, calling abort() on QNetworkReply
        // will immediately invoke the finished() signal handler
        // which under normal operation removes the submission
        // from the queue. so must take care not to invalidate
        // our iteration here at any point!

        std::partition_copy(
            std::begin(submits_), std::end(submits_),
            std::back_inserter(timeouts), std::back_inserter(pending),
            [](const submission& m) {
                return m.ticks == timeout_ticks;
            });

        std::for_each(std::begin(timeouts), std::end(timeouts),
            [](submission& m) {
                m.reply->abort();
            });
        submits_ = std::move(pending);

        if (submits_.empty())
        {
            killTimer(net_timeout_timer_);
            net_timeout_timer_ = 0;
        }
    }
    else
    {
        QObject::timerEvent(event);
    }
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

bool mainapp::submit_first()
{
    auto it = std::find_if(std::begin(submits_), std::end(submits_),
        [](const submission& sub) {
            return sub.reply == nullptr;
        });
    if (it == std::end(submits_))
        return false;

    auto& submit = *it;

    QNetworkRequest request;
    request.setRawHeader("User-Agent", "NewsflashPlus");
    
    submit.handler->prepare(request);
    submit.reply = net_.get(request);
    if (submit.reply == nullptr)
    {
        ERROR(str("Network get failed!"));
        //submit.handler->fa

        submits_.erase(it);
    }
    if (!net_timeout_timer_)
        net_timeout_timer_ = startTimer(1000);


    return true;
}


} // app
