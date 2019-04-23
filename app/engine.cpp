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

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QCoreApplication>
#  include <QEvent>
#  include <QFile>
#  include <QDir>
#  include <QBuffer>
#include "newsflash/warnpop.h"

#include "engine/ui/account.h"
#include "engine/nntp.h"
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
#include "types.h"
#include "download.h"
#include "platform.h"

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
    installEventFilter(this);
    ticktimer_ = startTimer(1000);

    DEBUG("Engine created");
}

Engine::~Engine()
{
    engine_.reset();

    DEBUG("Engine destroyed");
}

QString Engine::resolveDownloadPath(const QString& path, MainMediaType media) const
{
    if (!path.isEmpty())
        return path;
    auto it = downloads_.find(media);
    if (it == std::end(downloads_))
        return "";
    return it->second;
}

void Engine::testAccount(const Accounts::Account& acc)
{
    newsflash::ui::Account a;
    a.id                    = acc.id;
    a.name                  = toUtf8(acc.name);
    a.username              = toUtf8(acc.username);
    a.password              = toUtf8(acc.password);
    a.secure_host           = toLatin(acc.secureHost);
    a.general_host          = toLatin(acc.generalHost);
    a.secure_port           = acc.securePort;
    a.general_port          = acc.generalPort;
    a.enable_secure_server  = acc.enableSecureServer;
    a.enable_general_server = acc.enableGeneralServer;
    a.enable_compression    = acc.enableCompression;
    a.enable_pipelining     = acc.enablePipelining;
    a.connections = 1;
    engine_->TryAccount(a);
}

void Engine::setAccount(const Accounts::Account& acc)
{
    newsflash::ui::Account a;
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

    engine_->SetAccount(a);
}

void Engine::delAccount(const Accounts::Account& acc)
{
    // todo:
    //engine_.del(const newsflash::account &acc)
}

void Engine::setFillAccount(quint32 id)
{
    engine_->SetFillAccount(id);
}

bool Engine::downloadNzbContents(const Download& download, const QByteArray& nzb)
{
    QBuffer io(const_cast<QByteArray*>(&nzb));

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

    return downloadNzbContents(download, std::move(items));
}

bool Engine::downloadNzbContents(const Download& download, std::vector<NZBContent> nzb)
{
    Q_ASSERT(download.isValid());

    QString location = download.basepath;
    if (location.isEmpty())
    {
        auto it = downloads_.find(toMainType(download.type));
        if (it != std::end(downloads_))
            location = it->second;
    }

    location = joinPath(location, download.folder);

    QDir dir(location);
    if (!dir.mkpath(location))
    {
        ERROR("Error creating path %1", location);
        return false;
    }

    newsflash::ui::FileBatchDownload batch;
    batch.account = download.account;
    batch.path    = narrow(location);
    batch.desc    = toUtf8(download.desc);
    batch.size    = 0;
    for (auto& item : nzb)
    {
        newsflash::ui::FileDownload file;
        file.articles = std::move(item.segments);
        file.groups   = std::move(item.groups);
        file.size     = item.bytes;
        file.path     = batch.path;
        file.desc     = nntp::find_filename(toUtf8(item.subject));
        if (file.desc.size() < 5)
            file.desc = toUtf8(item.subject);
        batch.files.push_back(std::move(file));
        batch.size += item.bytes;
    }
    engine_->DownloadFiles(std::move(batch), download.priority);

    start();

    INFO("Downloading \"%1\"", download.desc);
    NOTE("Downloading \"%1\"", download.desc);

    emit newDownloadQueued(download);

    return true;
}

quint32 Engine::retrieveNewsgroupListing(quint32 acc)
{
    newsflash::ui::GroupListDownload listing;
    listing.account = acc;
    listing.desc    = toUtf8(tr("Download and update the list of available newsgroups"));

    const auto action = engine_->DownloadListing(listing);

    start();

    return action;
}

quint32 Engine::retrieveHeaders(quint32 acc, const QString& path, const QString& name)
{
    DEBUG("Header files for %1/%2", path, name);

    QDir dir(path);
    const auto location = joinPath(path, name);

    if (!dir.mkpath(location))
    {
        ERROR("Error creating path %1", location);
        return 0;
    }

    newsflash::ui::HeaderDownload update;
    update.account = acc;
    update.desc  = toUtf8(tr("Download and update headers for %1").arg(name));
    update.path  = narrow(path);
    update.group = toUtf8(name);

    const auto batchid = engine_->DownloadHeaders(update);

    start();

    INFO("Updating \"%1\"", name);
    NOTE("Updating \"%1\"", name);

    return batchid;
}


void Engine::loadState(Settings& s)
{
    logifiles_ = s.get("engine", "logfiles",
        QDir::toNativeSeparators(QDir::tempPath() + "/Newsflash"));

    // Before we had only one generic file path setting for all downloads
    // So if the user is migrating to a newer version we'll use this setting
    // for all the media type specific download paths.
    // if this setting doesn't exist then this is a new user without any previous
    // setting and then we default to the home path downloads.
    const QString& oldDownloadPath = s.get("engine", "downloads",
        QDir::toNativeSeparators(QDir::homePath() + "/Downloads"));
    if (!s.contains("engine", "download_path_for_adult"))
    {
        INFO("Updated download paths in the settings. Please review.");
        NOTE("Please review your settings.");
    }

    // initialize the media type specific paths.
    downloads_[MainMediaType::Adult]      = s.get("engine", "download_path_for_adult", oldDownloadPath);
    downloads_[MainMediaType::Apps]       = s.get("engine", "download_path_for_apps", oldDownloadPath);
    downloads_[MainMediaType::Music]      = s.get("engine", "download_path_for_music", oldDownloadPath);
    downloads_[MainMediaType::Games]      = s.get("engine", "download_path_for_games", oldDownloadPath);
    downloads_[MainMediaType::Images]     = s.get("engine", "download_path_for_images", oldDownloadPath);
    downloads_[MainMediaType::Movies]     = s.get("engine", "download_path_for_movies", oldDownloadPath);
    downloads_[MainMediaType::Television] = s.get("engine", "download_path_for_telly", oldDownloadPath);
    downloads_[MainMediaType::Other]      = s.get("engine", "download_path_for_other", oldDownloadPath);

    const auto overwrite = s.get("engine", "overwrite_existing_files", false);
    const auto discard   = s.get("engine", "discard_text", true);
    const auto secure    = s.get("engine", "prefer_secure", true);
    const auto throttle  = s.get("engine", "throttle", false);
    const auto throttleval = s.get("engine", "throttle_value", 5 * 1024);
    checkLowDisk_ = s.get("engine", "check_low_disk", checkLowDisk_);
    connect_      = s.get("engine", "connect", true);

    std::string logpath;

    QDir dir;
    if (!dir.mkpath(logifiles_))
    {
        ERROR("Error creating log path %1", logifiles_);
        WARN("Engine log files will may not be available.");
    }
    else
    {
        logpath = toUtf8(logifiles_);
    }

    engine_.reset(new newsflash::Engine(logpath));
    engine_->SetErrorCallback(std::bind(&Engine::onError, this,
        std::placeholders::_1));
    engine_->SetFileCallback(std::bind(&Engine::onFileComplete, this,
         std::placeholders::_1));
    engine_->SetTaskCallback(std::bind(&Engine::onTaskComplete, this,
        std::placeholders::_1));
    engine_->SetBatchCallback(std::bind(&Engine::onBatchComplete, this,
        std::placeholders::_1));
    engine_->SetUpdateCallback(std::bind(&Engine::onUpdateComplete, this,
        std::placeholders::_1));
    engine_->SetListCallback(std::bind(&Engine::onListComplete, this,
        std::placeholders::_1));
    engine_->SetHeaderInfoCallback(std::bind(&Engine::onHeaderInfoAvailable, this,
        std::placeholders::_1));
    engine_->SetFinishCallback(std::bind(&Engine::onAllComplete, this));
    engine_->SetQuotaCallback(std::bind(&Engine::onQuota, this,
        std::placeholders::_1, std::placeholders::_2));
    engine_->SetTestCallback(std::bind(&Engine::onConnectionTestComplete, this,
        std::placeholders::_1));
    engine_->SetTestLogCallback(std::bind(&Engine::onConnectionTestLogMsg, this,
        std::placeholders::_1));
    engine_->SetListingUpdateCallback(std::bind(&Engine::onListingUpdate, this,
        std::placeholders::_1));

    // remember that the notify callback can come from any thread
    // within the engine and it has to be thread safe.
    // so we simply post a notification to the main threads
    // event queue and then handle it in the eventFilter
    engine_->SetNotifyCallback([=]() {
        QCoreApplication::postEvent(this, new AsyncNotifyEvent);
    });

    engine_->SetOverwriteExistingFiles(overwrite);
    engine_->SetDiscardTextContent(discard);
    engine_->SetEnableThrottle(throttle);
    engine_->SetThrottleValue(throttleval);
    engine_->SetPreferSecure(secure);
}

void Engine::saveState(Settings& s)
{
    const auto overwrite = engine_->GetOverwriteExistingFiles();
    const auto discard   = engine_->GetDiscardTextContent();
    const auto secure    = engine_->GetPreferSecure();
    const auto throttle  = engine_->GetEnableThrottle();
    const auto throttleval = engine_->GetThrottleValue();

    s.set("engine", "download_path_for_adult", getDownloadPath(MainMediaType::Adult));
    s.set("engine", "download_path_for_apps",  getDownloadPath(MainMediaType::Apps));
    s.set("engine", "download_path_for_music", getDownloadPath(MainMediaType::Music));
    s.set("engine", "download_path_for_games", getDownloadPath(MainMediaType::Games));
    s.set("engine", "download_path_for_images", getDownloadPath(MainMediaType::Images));
    s.set("engine", "download_path_for_movies", getDownloadPath(MainMediaType::Movies));
    s.set("engine", "download_path_for_telly", getDownloadPath(MainMediaType::Television));
    s.set("engine", "download_path_for_other", getDownloadPath(MainMediaType::Other));

    s.set("engine", "throttle", throttle);
    s.set("engine", "throttle_value", throttleval);
    s.set("engine", "overwrite_existing_files", overwrite);
    s.set("engine", "discard_text", discard);
    s.set("engine", "prefer_secure", secure);
    s.set("engine", "logfiles", logifiles_);
    s.set("engine", "connect", connect_);
    s.set("engine", "check_low_disk", checkLowDisk_);
}

void Engine::loadSession()
{
    const auto& file = homedir::file("session.bin");
    try
    {
        if (QFileInfo(file).exists())
        {
            DEBUG("Loading engine session %1 ...", file);

            engine_->LoadTasks(toUtf8(file));
            if (engine_->GetNumTasks())
                connect(connect_);
        }
        DEBUG("Engine session loaded. Engine has %1 tasks", engine_->GetNumTasks());
    }
    catch (const std::exception& e)
    {
        ERROR("Failed to load previous session: '%1'", e.what());
    }
}

void Engine::saveSession()
{
    const auto& file = homedir::file("session.bin");

    DEBUG("Saving engine session in %1", file);

    engine_->SaveTasks(toUtf8(file));
}

void Engine::connect(bool on_off)
{
    connect_ = on_off;
    if (!connect_)
    {
        engine_->Stop();
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
    if (checkLowDisk_)
    {
        // todo:
    }
}

bool Engine::shutdown()
{
    killTimer(ticktimer_);
    shutdown_  = true;
    ticktimer_ = 0;

    engine_->Stop();

    if (!engine_->Pump())
        return true;

    DEBUG("Engine has pending actions");
    return false;
}

quint64 Engine::getBytesQueued(const QString& mountPoint)
{
    quint64 ret = 0;

    std::deque<newsflash::ui::TaskDesc> tasks;

    engine_->GetTasks(&tasks);

    for (const auto& task : tasks)
    {
        const auto& path  = widen(task.path);
        const auto& mount = resolveMountPoint(path);
        if (mount == mountPoint)
        {
            ret += task.size * (1.0 - (task.completion/100.0));
        }
    }

    return ret;
}

bool Engine::eventFilter(QObject* object, QEvent* event)
{
    if (object == this && event->type() == AsyncNotifyEvent::identity())
    {
        if (!engine_->Pump())
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
        const auto numPending = engine_->GetNumPendingTasks();

        //DEBUG("Num pending tasks %1", numPending);

        emit numPendingTasks(numPending);
        return true;
    }
    return QObject::eventFilter(object, event);
}

void Engine::timerEvent(QTimerEvent* event)
{
    if (shutdown_)
        return;
    // service the engine periodically
    if (engine_)
        engine_->Tick();
}

void Engine::start()
{
    if (connect_)
    {
        engine_->Start();
    }
}

void Engine::onError(const newsflash::ui::SystemError& error)
{
    //const auto resource = from_utf8(e.resource);
    //const auto code = e.code;
    ERROR("%1: %2", fromUtf8(error.resource), fromUtf8(error.what));
}

void Engine::onTaskComplete(const newsflash::ui::TaskDesc& task)
{
    using States = newsflash::ui::TaskDesc::States;
    Q_ASSERT(task.state == States::Error ||
             task.state == States::Complete);

    const auto& desc = widen(task.desc);

    DEBUG("Task \"%1\" completion %2", desc, task.completion);

    if (task.state == States::Error)
    {
        ERROR("\"%1\" encountered an error.", desc);
    }
    else if (task.error.any_bit())
    {
        if (task.completion > 0.0)
        {
            WARN("Task \"%1\" is incomplete.", desc);
            NOTE("Task \"%1\" is incomplete.", desc);
        }
        else
        {
            WARN("File \"%1\" is no longer available.", desc);
            NOTE("File \"%1\" is no longer available.", desc);
        }
    }
    else
    {
        INFO("Task \"%1\" is complete.", desc);
        NOTE("Task \"%1\" is complete.", desc);
    }
}

void Engine::onFileComplete(const newsflash::ui::FileResult& result)
{
    for (const auto& file : result.files)
    {
        QString name = widen(file.name);
        QString path = widen(file.path);
        if (path.isEmpty())
            path = QDir::currentPath();

        path = QDir(path).absolutePath();
        path = QDir::toNativeSeparators(path);
        DEBUG("File complete \"%1/%2\" damaged: %3 binary: %4",
            path, name, file.damaged, file.binary);

        app::FileInfo info;
        info.binary  = file.binary;
        info.damaged = file.damaged;
        info.name    = name;
        info.path    = path;
        info.size    = file.size;
        info.type    = findFileType(info.name);

        if (file.damaged)
        {
            WARN("File \"%1\" is damaged.", file.name);
            NOTE("File \"%1\" is damaged.", file.name);
        }
        else
        {
            INFO("File \"%1\" is complete.", file.name);
            NOTE("File \"%1\" is complete.", file.name);
        }

        emit fileCompleted(info);
    }
}

void Engine::onBatchComplete(const newsflash::ui::FileBatchResult& batch)
{
    QString path = widen(batch.path);

    DEBUG("Batch complete \"%1\"", path);

    path = QDir(path).absolutePath();
    path = QDir::toNativeSeparators(path);

    FilePackInfo pack;
    pack.desc     = fromUtf8(batch.desc);
    pack.path     = path;
    pack.numFiles = batch.filecount;
    pack.damaged  = batch.damaged;

    emit packCompleted(pack);
}

void Engine::onListComplete(const newsflash::ui::GroupListResult& list)
{
    DEBUG("Listing complete");

    QList<NewsGroupInfo> infoList;

    for (const auto& group : list.groups)
    {
        NewsGroupInfo groupInfo;
        groupInfo.first = group.first;
        groupInfo.last  = group.last;
        groupInfo.size  = group.size;
        groupInfo.name  = fromUtf8(group.name);
        infoList.append(groupInfo);
    }

    emit listCompleted(list.account, infoList);
}

void Engine::onUpdateComplete(const newsflash::ui::HeaderResult& result)
{
    HeaderInfo info;
    info.groupName = fromUtf8(result.group);
    info.groupPath = fromUtf8(result.path);
    info.numLocalArticles = result.num_local_articles;
    info.numRemoteArticles = result.num_remote_articles;
    DEBUG("%1 Update complete at %2", info.groupName, info.groupPath);

    INFO("%1 updated with %2 articles of %3 available", info.groupName,
        info.numLocalArticles, info.numRemoteArticles);
    NOTE("Updated %1", info.groupName);

    emit updateCompleted(info);
}

void Engine::onHeaderInfoAvailable(const newsflash::ui::HeaderUpdate& update)
{
    for (size_t i=0; i<update.catalogs.size(); ++i)
    {
        HeaderUpdateInfo info;
        info.groupName = fromUtf8(update.group_name);
        info.groupFile = fromUtf8(update.catalogs[i]);
        info.numLocalArticles  = update.num_local_articles;
        info.numRemoteArticles = update.num_remote_articles;
        info.snapshot = update.snapshots[i];
        info.account  = update.account;
        emit newHeaderInfoAvailable(info);
    }
}

void Engine::onListingUpdate(const newsflash::ui::GroupListUpdate& update)
{
    QList<NewsGroupInfo> infoList;

    for (const auto& group : update.groups)
    {
        NewsGroupInfo groupInfo;
        groupInfo.first = group.first;
        groupInfo.last  = group.last;
        groupInfo.size  = group.size;
        groupInfo.name  = fromUtf8(group.name);
        infoList.append(groupInfo);
    }

    emit listUpdated(update.account, infoList);
}

void Engine::onAllComplete()
{
    DEBUG("All downloads are complete.");
    emit allCompleted();
}

void Engine::onQuota(std::size_t bytes, std::size_t account)
{
    DEBUG("Quota update %1 for account %2", app::size{bytes}, account);

    emit quotaUpdate(bytes, account);
}

void Engine::onConnectionTestComplete(bool success)
{
    DEBUG("Connection test complete: %1", success);

    emit testAccountComplete(success);
}

void Engine::onConnectionTestLogMsg(const std::string& msg)
{
    DEBUG("Connection test log: '%1'", msg);

    emit testAccountLogMsg(fromUtf8(msg));
}

Engine* g_engine;


} // app
