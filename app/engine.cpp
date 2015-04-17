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

#define LOGTAG "engine"

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QCoreApplication>
#  include <QEvent>
#  include <QFile>
#  include <QDir>
#  include <QBuffer>
#include <newsflash/warnpop.h>
#include <newsflash/engine/ui/account.h>
#include <newsflash/engine/nntp.h>
#include "engine.h"
#include "debug.h"
#include "homedir.h"
#include "format.h"
#include "eventlog.h"
#include "account.h"
#include "settings.h"
#include "nzbparse.h"
#include "fileinfo.h"
#include "newsinfo.h"
#include "utility.h"

namespace nf = newsflash;
namespace ui = newsflash::ui;

namespace {
    // this event object is posted to the application's
    // event queue when the core engine has pending actions.
    // the callback from the engine comes from anothread thread
    // and we post the event to the app's main thread's event
    // queue which then picks up the event and calls the engine's 
    // message dispatcher.
    class AsyncNotifyEvent : public QEvent
    {
    public:
        AsyncNotifyEvent() : QEvent(identity())
        {}
       ~AsyncNotifyEvent()
        {}

        static QEvent::Type identity() 
        {
            static auto id = QEvent::registerEventType();
            return (QEvent::Type)id;
        }
    private:
    };
} // namespace

namespace app
{

Engine::Engine()
{
    diskspace_  = 0;
    totalspeed_ = 0;
    shutdown_  = false;

    engine_.reset(new newsflash::engine);
    engine_->set_error_callback(std::bind(&Engine::onError, this,
        std::placeholders::_1));
    engine_->set_file_callback(std::bind(&Engine::onFileComplete, this,
         std::placeholders::_1));
    engine_->set_batch_callback(std::bind(&Engine::onBatchComplete, this,
        std::placeholders::_1));
    engine_->set_update_callback(std::bind(&Engine::onUpdateComplete, this,
        std::placeholders::_1));
    engine_->set_list_callback(std::bind(&Engine::onListComplete, this,
        std::placeholders::_1));
    engine_->set_header_data_callback(std::bind(&Engine::onHeaderDataAvailable, this,
        std::placeholders::_1));
    engine_->set_header_info_callback(std::bind(&Engine::onHeaderInfoAvailable, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    // remember that the notify callback can come from any thread
    // within the engine and it has to be thread safe.
    // so we simply post a notification to the main threads
    // event queue and then handle it in the eventFilter
    engine_->set_notify_callback([=]() {
        QCoreApplication::postEvent(this, new AsyncNotifyEvent);
    });

    installEventFilter(this);
    ticktimer_ = startTimer(1000);

    DEBUG("Engine created");
}

Engine::~Engine()
{
    engine_.reset();

    DEBUG("Engine destroyed");
}

void Engine::setAccount(const Accounts::Account& acc)
{
    newsflash::ui::account a;
    a.id                    = acc.id;
    a.name                  = toUtf8(acc.name);
    a.username              = toUtf8(acc.username);
    a.password              = toUtf8(acc.password);
    a.secure_host           = toLatin(acc.secureHost);
    a.general_host          = toLatin(acc.generalHost);    
    a.secure_port           = acc.securePort;    
    a.general_port          = acc.generalPort;    
    a.enable_compression    = acc.enableCompression;
    a.enable_pipelining     = acc.enablePipelining;
    a.enable_general_server = acc.enableGeneralServer;
    a.enable_secure_server  = acc.enableSecureServer;
    a.connections           = acc.maxConnections;

    engine_->set_account(a);
}

void Engine::delAccount(const Accounts::Account& acc)
{
    // todo:
    //engine_.del(const newsflash::account &acc)
}

void Engine::setFillAccount(quint32 id)
{
    engine_->set_fill_account(id);
}

bool Engine::downloadNzbContents(quint32 acc, const QString& basePath, const QString& path, const QString& desc, const QByteArray& buff)
{
    QBuffer io(const_cast<QByteArray*>(&buff));

    std::vector<NZBContent> items;

    const auto err = parseNZB(io, items);
    switch (err)
    {
        case NZBError::None: break;

        case NZBError::XML:
            ERROR("NZB XML parse error.");
            break;

        case NZBError::NZB:
            ERROR("NZB content error.");
            break;

        default:
            ERROR("Error reading NZB.");
            break;
    }
    if (err != NZBError::None)
        return false;

    auto location = basePath.isEmpty() ? 
        downloads_ : basePath;
    location = joinPath(location, path);

    QDir dir(location);
    if (!dir.mkpath(location))
    {
        ERROR("Error creating path %1", location);
        return false;
    }

    newsflash::ui::batch download;
    download.account = acc;
    download.path = narrow(location);
    download.desc = toUtf8(desc);
    for (auto& item : items)
    {
        newsflash::ui::download file;
        file.articles = std::move(item.segments);
        file.groups   = std::move(item.groups);
        file.size     = item.bytes;
        file.name     = nntp::find_filename(toUtf8(item.subject));
        if (file.name.size() < 5)
            file.name = toUtf8(item.subject);
        download.files.push_back(std::move(file));
    }
    engine_->download_files(std::move(download));

    start();

    INFO("Downloading \"%1\"", desc);
    NOTE("Downloading \"%1\"", desc);

    emit newDownloadQueued(desc);
    return true;
}


bool Engine::downloadNzbContents(quint32 acc, const QString& basePath, const QString& path, const QString& desc, std::vector<NZBContent> nzb)
{
    auto location = basePath.isEmpty() ?
        downloads_ : basePath;
    location = joinPath(location, path);

    QDir dir(location);
    if (!dir.mkpath(location))
    {
        ERROR("Error creating path %1", location);
        return false;
    }

    newsflash::ui::batch download;
    download.account = acc;
    download.path    = narrow(location);
    download.desc    = toUtf8(desc);
    for (auto& item : nzb)
    {
        newsflash::ui::download file;
        file.articles = std::move(item.segments);
        file.groups   = std::move(item.groups);
        file.size     = item.bytes;
        file.name     = nntp::find_filename(toUtf8(item.subject));
        if (file.name.size() < 5)
            file.name = toUtf8(item.subject);
        download.files.push_back(std::move(file));
    }
    engine_->download_files(std::move(download));

    start();

    INFO("Downloading \"%1\"", desc);
    NOTE("Downloading \"%1\"", desc);

    emit newDownloadQueued(desc);
    return true;
}

quint32 Engine::retrieveNewsgroupListing(quint32 acc)
{
    newsflash::ui::listing listing;
    listing.account = acc;
    listing.desc    = toUtf8(tr("Newsgroup Listing"));

    const auto action = engine_->download_listing(listing);

    start();

    return action;
}

quint32 Engine::retrieveHeaders(quint32 acc, const QString& path, const QString& name)
{
    DEBUG("Header files for %1/%2", path, name);

    QDir dir(path);
    if (!dir.mkpath(path + "/" + name))
    {
        ERROR("Error creating path %1", path);
        return 0;
    }

    newsflash::ui::update update;
    update.account = acc;
    update.desc = toUtf8(tr("Get headers for %1").arg(name));
    update.path = narrow(path);
    update.name = toUtf8(name);

    const auto batchid = engine_->download_headers(update);

    start();

    INFO("Updating \"%1\"", name);
    NOTE("Updating \"%1\"", name);

    return batchid;
}


void Engine::loadState(Settings& s)
{
    logifiles_ = s.get("engine", "logfiles", 
        QDir::toNativeSeparators(QDir::tempPath() + "/Newsflash"));
    downloads_ = s.get("engine", "downloads", 
        QDir::toNativeSeparators(QDir::homePath() + "/Downloads"));

    mountpoint_ = resolveMountPoint(downloads_);

    const auto overwrite = s.get("engine", "overwrite_existing_files", false);
    const auto discard   = s.get("engine", "discard_text", true);
    const auto secure    = s.get("engine", "prefer_secure", true);
    const auto throttle  = s.get("engine", "throttle", false);
    const auto throttleval = s.get("engine", "throttle_value", 0);
    connect_ = s.get("engine", "connect", true);

    engine_->set_overwrite_existing_files(overwrite);
    engine_->set_discard_text_content(discard);
    engine_->set_throttle(throttle);
    engine_->set_throttle_value(throttleval);
    engine_->set_prefer_secure(secure);

}

void Engine::saveState(Settings& s)
{
    const auto overwrite = engine_->get_overwrite_existing_files();
    const auto discard   = engine_->get_discard_text_content();
    const auto secure    = engine_->get_prefer_secure();
    const auto throttle  = engine_->get_throttle();
    const auto throttleval = engine_->get_throttle_value();

    s.set("engine", "throttle", throttle);
    s.set("engine", "throttle_value", throttleval);
    s.set("engine", "overwrite_existing_files", overwrite);
    s.set("engine", "discard_text", discard);
    s.set("engine", "prefer_secure", secure);
    s.set("engine", "logfiles", logifiles_);
    s.set("engine", "downloads", downloads_);
    s.set("engine", "connect", connect_);
}

void Engine::loadSession()
{
    const auto& file = homedir::file("session.bin");
    try
    {
        if (QFileInfo(file).exists())
        {
            DEBUG("Loading engine session %1 ...", file);
            
            engine_->load_session(toUtf8(file));
            if (engine_->num_tasks())
                connect(connect_);
        }
        DEBUG("Engine session loaded. Engine has %1 tasks", engine_->num_tasks());
    }
    catch (const std::exception& e)
    {
        WARN("Failed to load previous session: '%1'", e.what());
    }
}

void Engine::saveSession()
{
    const auto& file = homedir::file("session.bin");

    DEBUG("Saving engine session in %1", file);

    engine_->save_session(toUtf8(file));
}

void Engine::connect(bool on_off)
{ 
    connect_ = on_off;
    if (!connect_)
    {
        engine_->stop();
    }
    else
    {
        start();
    }
}

void Engine::refresh()
{
    // rescan the default download location volume
    // for free space. note that the location might 
    // point to a folder that doesn't yet exist.. 
    // so traverse the path towards the root untill 
    // an existing path is found.
    diskspace_ = app::getFreeDiskSpace(mountpoint_);
}

bool Engine::shutdown()
{
    killTimer(ticktimer_);    
    shutdown_  = true;
    ticktimer_ = 0;

    engine_->stop();

    if (!engine_->pump())
        return true;

    DEBUG("Engine has pending actions");
    return false;
}

bool Engine::eventFilter(QObject* object, QEvent* event)
{
    if (object == this && event->type() == AsyncNotifyEvent::identity())
    {
        if (!engine_->pump())
        {
            if (shutdown_)
            {
                DEBUG("Shutdown complete");
                // this signal can actually happen multiple times since
                // the pump function processes all currently queued state transitions
                // yet we may have events queued in the application event queue:
                // this should be completely harmless though.
                emit shutdownComplete();
            }
        }
        return true;
    }
    return QObject::eventFilter(object, event);
}

void Engine::timerEvent(QTimerEvent* event)
{
    if (shutdown_)
        return;
    // service the engine periodically
    engine_->tick();
}

void Engine::start()
{
    if  (connect_)
    {
        QDir dir;
        if (!dir.mkpath(logifiles_))
        {
            WARN("Error creating log path %1", logifiles_);
            WARN("Engine log files will not be available.");
        }
        engine_->start(toUtf8(logifiles_));
    }
}

void Engine::onError(const newsflash::ui::error& e)
{
    //const auto resource = from_utf8(e.resource);
    //const auto code = e.code;
    ERROR("%1: %2", fromUtf8(e.resource), fromUtf8(e.what));
}

void Engine::onFileComplete(const newsflash::ui::file& f)
{
    QString name = widen(f.name);
    QString path = widen(f.path);
    if (path.isEmpty())
        path = QDir::currentPath();

    path = QDir(path).absolutePath();
    path = QDir::toNativeSeparators(path);
    DEBUG("File complete \"%1/%2\" damaged: %3 binary: %4", 
        path, name, f.damaged, f.binary);

    app::FileInfo file;
    file.binary  = f.binary;
    file.damaged = f.damaged;
    file.name    = name;
    file.path    = path;
    file.size    = f.size;
    file.type    = findFileType(file.name);

    if (f.damaged)
    {
        WARN("\"%1\" is damaged.", file.name);
    }
    else
    {
        INFO("Completed \"%1\".", file.name);
    }

    emit fileCompleted(file);

    NOTE("\"%1\" is ready", file.name);
}

void Engine::onBatchComplete(const newsflash::ui::batch& b)
{
    QString path = widen(b.path);

    DEBUG("Batch complete \"%1\"", path);

    path = QDir(path).absolutePath();
    path = QDir::toNativeSeparators(path);

    FilePackInfo pack;
    pack.desc = fromUtf8(b.desc);
    pack.path = path;

    emit packCompleted(pack);
}

void Engine::onListComplete(const newsflash::ui::listing& l)
{
    DEBUG("Listing complete");

    QList<NewsGroupInfo> list;

    for (const auto& ui : l.groups)
    {
        NewsGroupInfo group;
        group.first = ui.first;
        group.last  = ui.last;
        group.size  = ui.size;
        group.name  = fromUtf8(ui.name);
        list.append(group);
    }

    emit listCompleted(l.account, list);
}

void Engine::onUpdateComplete(const newsflash::ui::update& u)
{
    HeaderInfo info;
    info.groupName = fromUtf8(u.name);
    info.groupPath = fromUtf8(u.path);
    info.numLocalArticles = u.num_local_articles;
    info.numRemoteArticles = u.num_remote_articles;
    DEBUG("%1 Update complete at %2", info.groupName, info.groupPath);

    INFO("%1 updated with %2 articles of %3 available", info.groupName, 
        info.numLocalArticles, info.numRemoteArticles);
    NOTE("Updated %1", info.groupName);

    emit updateCompleted(info);
}

void Engine::onHeaderDataAvailable(const std::string& file)
{
    emit newHeaderDataAvailable(widen(file));
}

void Engine::onHeaderInfoAvailable(const std::string& group,
    std::uint64_t numLocal, std::uint64_t numRemote)
{
    emit newHeaderInfoAvailable(fromUtf8(group), numLocal, numRemote);
}

Engine* g_engine;


} // app
    