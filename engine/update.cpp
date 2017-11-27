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

#include "newsflash/config.h"

#include <limits>
#include <fstream>
#include <map>
#include <set>
#include <mutex>

#include "filesys.h"
#include "linebuffer.h"
#include "filemap.h"
#include "filebuf.h"
#include "update.h"
#include "buffer.h"
#include "action.h"
#include "nntp.h"
#include "utf8.h"
#include "logging.h"
#include "cmdlist.h"
#include "catalog.h"
#include "idlist.h"
#include "filesys.h"
#include "assert.h"

namespace newsflash
{

using catalog_t   = catalog<filebuf>;
using article_t   = article<filebuf>;
using idlist_t = idlist<filebuf>;

#pragma pack(push, 1)
// these are just an internal helper structure that we use to read
// and write the hash structure to file
struct FileHash {
    std::uint32_t article_hash = 0;
    std::uint64_t article_number_internal = 0;
};

enum {
    CurrentVersion = 1
};

struct FileHeader {
    std::uint8_t  version = 0;
    std::uint64_t local_first = 0;
    std::uint64_t local_last  = 0;
    std::uint64_t landmark_article_number = 0;
    std::int64_t  positive_offset = 0;
    std::int64_t  negative_offset = 0;
};

#pragma pack(pop)

struct Update::state {
    std::string folder;
    std::string group;

    HeaderTask::OnProgress callback;

    // this mutex is acquired by any thread to get an
    // exclusive view to the the underlying data files.
    // i.e. holding this mutex makes sure that the files
    // will stay in a consistent state.
    std::mutex file_io_mutex;

    // maps a volume index to a file.
    std::map<std::uint32_t, std::unique_ptr<catalog_t>> files;

    // maps a hash value to a *internal* article number
    // we use a multimap so that we can resolve hash collisions
    std::multimap<std::uint32_t, std::uint64_t> hashmap;

    // message id db
    idlist_t idb;

    // this mutex is to eliminate the possible race condition
    // between the tasks's commit operation and some possible
    // enqueued operations that would like to modify the data files.
    std::mutex cancel_operation_mutex;
    // cancel rest of the enqueued operations.
    bool cancel_operation = false;

    // these variables track our internal article numbering.
    // landmark defines the very first article number we have
    // received and internally it maps to itself.
    // articles newer (bigger) than landmark are given internal number landmark + positive_offset
    // while articles older (smaller) are given landmark + negative_ offset
    //
    // these variables are in the state object since each store action
    // needs to access the latest data, i.e. the state is global
    // and not something each task can replicate on it's own_
    // these are not procted by a mutex since the store
    // actions execute sequentially.
    // if store actions are multithreaded then these need to be mutexed.
    std::uint64_t landmark_article_number = 0;
    std::int64_t  positive_offset = 0;
    std::int64_t  negative_offset = 0;

    // allocate a new internal article id.
    std::uint64_t allocate_internal(const article_t& article)
    {
        if (!landmark_article_number)
        {
            landmark_article_number = article.number();
            return landmark_article_number;
        }
        std::uint64_t external_number = article.number();
        std::uint64_t internal_number = 0;
        if (external_number > landmark_article_number)
        {
            positive_offset++;
            internal_number = landmark_article_number + positive_offset;
        }
        else if (external_number < landmark_article_number)
        {
            negative_offset--;
            internal_number = landmark_article_number + negative_offset;
        }
        return internal_number;
    }

    std::string file_volume_name(std::size_t index) const
    {
        const auto max_index = std::numeric_limits<std::uint64_t>::max() / std::uint64_t(CATALOG_SIZE);

        std::string s;
        {
            std::stringstream ss;
            ss << max_index;
            ss >> s;
        }
        std::stringstream ss;
        ss << "vol" << std::setfill('0') << std::setw(s.size()) << index << ".dat";
        const auto base = fs::joinpath(folder, group);
        const auto file = fs::joinpath(base, ss.str());
        return file;
    }
};

// parse NNTP overview data
class Update::parse : public action
{
public:
    parse(Buffer&& buff) : buffer_(std::move(buff))
    {}

    virtual void xperform() override
    {
        nntp::linebuffer lines(buffer_.Content(), buffer_.GetContentLength());
        auto beg = lines.begin();
        auto end = lines.end();
        for (; beg != end; ++beg)
        {
            const auto& line = *beg;

            article_t a;
            if (!a.parse(line.start, line.length))
                continue;

            articles_.push_back(a);
        }
    }
    virtual std::size_t size() const override
    { return buffer_.GetContentLength(); }

	virtual std::string describe() const override
	{ return "Parse XOVER"; }
private:
    friend class Update;
    std::shared_ptr<state> state_;
    std::vector<article_t> articles_;
private:
    Buffer buffer_;
};

class Update::store : public action
{
public:
    store(std::shared_ptr<state> s) : state_(s), first_(0), last_(0)
    {}

    virtual void xperform() override
    {
        std::lock_guard<std::mutex> cancel_lock(state_->cancel_operation_mutex);
        if (state_->cancel_operation)
            return;

        first_ = std::numeric_limits<decltype(first_)>::max();

        auto& files = state_->files;
        auto& hmap  = state_->hashmap;
        auto& idb   = state_->idb;

        for (std::size_t i=0; i<articles_.size(); ++i)
        {
            auto& article = articles_[i];

            last_  = std::max(last_, article.number());
            first_ = std::min(first_, article.number());

            std::uint64_t internal_article_number = 0;

            if (article.has_parts())
            {
                const auto hash = article.hash();
                // if it's a multipart see if we know the hash value
                // already and can map it to a article number that way
                // but since it's possible that there are hash collisions
                // we need to go over the potential hash matches and check
                // that they actually match.
                auto beg = hmap.lower_bound(hash);
                auto end = hmap.upper_bound(hash);
                for (; beg != end; ++beg)
                {
                    const auto number = beg->second;
                    const auto file_index  = number / CATALOG_SIZE;
                    const auto file_bucket = number & (CATALOG_SIZE-1);
                    const auto file_name   = state_->file_volume_name(file_index);
                    if (!fs::exists(file_name))
                    {
                        // if the file doesn't exist anymore then whole volume
                        // has probably been purged in which case we just discard the hash
                        beg = hmap.erase(beg);
                        continue;
                    }
                    auto it = files.find(file_index);
                    if (it == std::end(files))
                    {
                        std::unique_ptr<catalog_t> db(new catalog_t);
                        db->open(file_name);
                        it = files.insert(std::make_pair(file_index, std::move(db))).first;
                    }
                    const auto& db    = it->second;
                    const auto index  = catalog_t::index_t(file_bucket);
                    const auto& other = db->load(index);
                    if (other.is_match(article))
                    {
                        internal_article_number = number;
                        break;
                    }
                }
                if (!internal_article_number)
                {
                    internal_article_number = state_->allocate_internal(article);
                    hmap.insert(std::make_pair(article.hash(), internal_article_number));
                }
            }
            else
            {
                internal_article_number = state_->allocate_internal(article);
            }


            ASSERT(internal_article_number  && "Article number is undefined.");

            const auto file_index  = internal_article_number / CATALOG_SIZE;
            const auto file_bucket = internal_article_number & (CATALOG_SIZE-1);
            auto it = files.find(file_index);
            if (it == std::end(files))
            {
                std::unique_ptr<catalog_t> db(new catalog_t);
                db->open(state_->file_volume_name(file_index));
                it = files.insert(std::make_pair(file_index, std::move(db))).first;
            }
            auto& db = it->second;

            const auto index = catalog_t::index_t(file_bucket);
            if (!db->is_empty(index))
            {
                auto a = db->load(index);
                ASSERT(a.is_match(article));
                ASSERT(a.has_parts());

                const auto max_parts = a.num_parts_total();
                const auto num_part  = article.partno();
                if (max_parts == 1)
                    continue;
                if (num_part > max_parts)
                    continue;

                const auto base = a.number();
                const auto num  = article.number();
                std::int16_t diff = 0;
                if (base > num)
                    diff -= (std::int16_t)(base - num);
                else diff = (std::int16_t)(num - base);
                idb[a.idbkey() + num_part] = diff;

                a.combine(article);
                db->update(a, index);
            }
            else
            {
                article.set_index(index.value );
                // we store one complete 64bit article number for the whole pack
                // and then for the additional parts we store a 16 bit delta value.
                // note that while yenc generally uses 1 based part indexing some
                // posters use 0 based instead. Hence we just add + 1 to cater for
                // both cases safely.
                if (article.has_parts())
                {
                    const auto key = idb.size();

                    article.set_idbkey(key);
                    idb.resize(idb.size() + article.num_parts_total() + 1);
                    idb[key + article.partno()] = 0; // 0 difference to the message id stored with the article.
                }
                db->insert(article, index);
            }
            updates_.insert(db.get());
        }

        // important: we take the mutex here to acquire exclusive access to the
        // files. when we hold the lock the UI should *not* be reloading the
        // catalog files since their state may be inconsistent.
        // instead the UI *must* only do that in a response to on_write callback
        // since during the execution of that callback the UI holds this mutex.
        std::lock_guard<std::mutex> lock(state_->file_io_mutex);

        // write out changes to the disk
        idb.flush();

        for (auto* db : updates_)
            db->flush();
    }

    virtual std::string describe() const override
    { return "Update Db"; }

    virtual std::size_t size() const override
    { return bytes_; }
private:
    friend class Update;
    std::shared_ptr<state> state_;
    std::vector<article_t> articles_;
    std::set<catalog_t*> updates_;
private:
    std::uint64_t first_ = 0;
    std::uint64_t last_  = 0;
private:
    std::size_t bytes_ = 0;
};

Update::Update(const std::string& path, const std::string& group)
{
    const auto nfo = fs::joinpath(fs::joinpath(path, group), group + ".nfo");
    const auto idb = fs::joinpath(fs::joinpath(path, group), group + ".idb");

    state_ = std::make_shared<state>();
    state_->folder = path;
    state_->group  = group;

#if defined(LINUX_OS)
    std::ifstream in(nfo, std::ios::in | std::ios::binary);
#elif defined(WINDOWS_OS)
    // msvc extension
    std::ifstream in(utf8::decode(nfo), std::ios::in | std::ios::binary);
#endif
    if (in.is_open())
    {
        in.seekg(0, std::ios::end);
        const auto total_size   = (unsigned long)in.tellg();
        const auto header_size  = sizeof(FileHeader);
        const auto payload_size = total_size - header_size;
        const auto num_hashes   = payload_size / sizeof(FileHash);
        in.seekg(0, std::ios::beg);

        // previously there was no versioning of the meta database file
        // but if there was a need to change the structure the catalog
        // version would change and then in update task an exception
        // would occur when opening a catalog object.
        // now we're adding a header here. the version check is a bit iffy
        // since in case of an older data file we're just reading a "junk"
        // version value. However in practice this might not be such a big deal
        // since the UI has implicitly done a version check through the catalog
        // version when opening the group and has (thus) failed.
        FileHeader header;
        in.read((char*)&header, sizeof(header));
        if (header.version != CurrentVersion)
            throw std::runtime_error("unsupported file version");

        assert((payload_size % sizeof(FileHash)) == 0);

        local_first_ = header.local_first;
        local_last_  = header.local_last;
        state_->landmark_article_number = header.landmark_article_number;
        state_->positive_offset = header.positive_offset;
        state_->negative_offset = header.negative_offset;

        if (num_hashes)
        {
            // read back the hashes
            std::vector<FileHash> vec;
            vec.resize(num_hashes);
            in.read((char*)&vec[0], num_hashes * sizeof(FileHash));
            for (std::size_t i=0; i<vec.size(); ++i)
            {
                const auto key = vec[i].article_hash;
                const auto val = vec[i].article_number_internal;
                state_->hashmap.insert(std::make_pair(key, val));
            }
        }
        xover_last_  = local_last_;
        xover_first_ = local_first_;
    }
    else
    {
        local_last_  = 0;
        local_first_ = std::numeric_limits<decltype(local_first_)>::max();
        xover_first_ = 0;
        xover_last_  = 0;
    }
    remote_first_ = 0;
    remote_last_  = 0;

    state_->idb.open(idb);
}

Update::~Update()
{
    // so unless the data is flushed out and written
    // to the disk we end up with inconsistent state.
    // a simple "solution" for now is to make sure that
    // we always perform the commit, i.e. we'll do it
    // in the ctor. this however has the problem that if
    // something goes wrong (and an exception is thrown)
    // we have to catch it and there's no way to tell
    // the user about the problem.
    //
    // we really need to refactor the engine shutdown
    // sequence so that the tasks are shutdown
    // and any problems can be propagated up to the UI layer.
    try
    {
        Commit();
    }
    catch (const std::exception& e)
    {}
}

std::shared_ptr<CmdList> Update::CreateCommands()
{
    std::shared_ptr<CmdList> ret;

    if (remote_last_ == 0)
    {
        // get group information for retrieving the first and last
        // article numbers on the remote server.
        ret.reset(new CmdList(CmdList::GroupInfo{state_->group}));
    }
    else if (xover_last_ < remote_last_)
    {
        CmdList::Overviews ov;
        ov.group = state_->group;

        // while there are newer messages on the remote host we generate
        // commands to retrieve the newer messages starting at the current
        // newest message on the local machine and progressing towards the newest
        // on the remote host.
        for (int i=0; i<10; ++i)
        {
            const auto first = xover_last_ + 1;
            const auto last  = std::min(first + 999, remote_last_);
            xover_last_ = last;
            std::string range;
            std::stringstream ss;
            ss << first << "-" << last;
            ss >> range;
            ov.ranges.push_back(std::move(range));
            if (last == remote_last_)
                break;
        }
        ret.reset(new CmdList(ov));
    }
    else if (xover_first_ > remote_first_)
    {
        CmdList::Overviews ov;
        ov.group = state_->group;

        // while there are older messages older on the remote host we generate
        // commands to retrieve the older messges starting at the current oldest
        // message on the local machine and progressing towards the oldest on the
        // remote server.
        for (int i=0; i<10; ++i)
        {
            const auto last  = xover_first_ - 1;
            const auto first = last - remote_first_ >= 999 ?  last - 999 : remote_first_;
            xover_first_ = first;
            std::string range;
            std::stringstream ss;
            ss << first << "-" << last;
            ss >> range;
            ov.ranges.push_back(std::move(range));
            if (first == remote_first_)
                break;
        }
        ret.reset(new CmdList(ov));
    }

    return ret;
}

void Update::Cancel()
{
    // the update is a little bit special.
    // we always want to retain the headers that've we grabbed
    // instead of having to wait untill the end of data before committing.
    Commit();
}

void Update::Commit()
{
    if (commit_done_)
        return;

    // there might be pending operations to write more data to the
    // data files, but we're just going to turn them into non-ops
    std::lock_guard<std::mutex> lock(state_->cancel_operation_mutex);
    state_->cancel_operation = true;

    const auto& path  = state_->folder;
    const auto& group = state_->group;
    const auto file = fs::joinpath(fs::joinpath(path, group), group + ".nfo");

    if (local_first_ == 0 || local_last_ == 0)
        return;

#if defined(LINUX_OS)
    std::ofstream out(file, std::ios::out | std::ios::binary | std::ios::trunc);
#elif defined(WINDOWS_OS)
    std::ofstream out(utf8::decode(file), std::ios::out | std::ios::binary | std::ios::trunc);
#endif

    if (!out.is_open())
        throw std::runtime_error("unable to open: " + file);

    FileHeader header;
    header.version = CurrentVersion;
    header.local_first = local_first_;
    header.local_last  = local_last_;
    header.landmark_article_number = state_->landmark_article_number;
    header.positive_offset = state_->positive_offset;
    header.negative_offset = state_->negative_offset;

    out.write((const char*)&header, sizeof(header));

    auto& files = state_->files;
    for (auto& p : files)
    {
        auto& db = p.second;
        db->flush();
    }

    std::vector<FileHash> vec;
    const auto& hashmap = state_->hashmap;
    vec.resize(hashmap.size());
    size_t i=0;
    for (const auto& pair : hashmap)
    {
        vec[i].article_hash = pair.first;
        vec[i].article_number_internal = pair.second;
        ++i;
    }
    out.write((const char*)&vec[0], vec.size() * sizeof(FileHash));
    out.close();

    commit_done_ = true;
}

void Update::Complete(CmdList& cmd, std::vector<std::unique_ptr<action>>& next)
{
    if (cmd.GetType() == CmdList::Type::GroupInfo)
    {
        const auto& buffer   = cmd.GetBuffer(0);
        const auto& pair = nntp::parse_group(buffer.Content(), buffer.GetContentLength());
        if (!pair.first)
            throw std::runtime_error("parse group information failed");

        const auto& group = pair.second;

        remote_first_ = std::stoull(group.first);
        remote_last_  = std::stoull(group.last);
        if (xover_last_ == 0)
        {
            xover_last_  = remote_last_;
            xover_first_ = remote_last_ + 1;
        }

        LOG_I(state_->group, " first ", remote_first_, " last ", remote_last_);
    }
    else
    {
        for (std::size_t i=0; i<cmd.NumBuffers(); ++i)
        {
            auto& buffer = cmd.GetBuffer(i);
            if (buffer.GetContentStatus() != Buffer::Status::Success)
                continue;

            std::unique_ptr<action> p(new parse(std::move(buffer)));
            p->set_affinity(action::affinity::any_thread);
            next.push_back(std::move(p));
        }
    }
}

void Update::Complete(action& a, std::vector<std::unique_ptr<action>>& next)
{
    if (auto* p = dynamic_cast<parse*>(&a))
    {
        std::unique_ptr<store> s(new store(state_));
        s->articles_ = std::move(p->articles_);
        s->bytes_ = p->size();
        s->set_affinity(action::affinity::single_thread);
        next.push_back(std::move(s));
    }
    if (auto* p = dynamic_cast<store*>(&a))
    {
        const auto first = p->first_;
        const auto last  = p->last_;
        local_first_ = std::min(local_first_, first);
        local_last_  = std::max(local_last_, last);

        if (state_->callback)
        {
            HeaderTask::Progress progress;
            progress.group = state_->group;
            progress.path  = state_->folder;
            progress.num_local_articles  = local_last_ - local_first_ + 1;
            progress.num_remote_articles = remote_last_ - remote_first_ + 1;
            for (auto* catalog : p->updates_)
            {
                std::unique_ptr<Snapshot> snapshot = catalog->snapshot();
                progress.catalogs.push_back(catalog->device().filename());
                progress.snapshots.push_back(std::move(snapshot));
            }
            state_->callback(progress);
        }
    }
}

bool Update::HasCommands() const
{
    if (remote_first_ == 0 && remote_last_  == 0)
        return true;
    //return (remote_last_ != xover_last_) || (remote_first_ != xover_first_);
    return xover_last_ < remote_last_ || xover_first_ > remote_first_;
}

std::size_t Update::MaxNumActions() const
{
    if (remote_last_ == 0)
        return 0;

    const auto remote_articles = remote_last_ - remote_first_ + 1;
    const auto local_articles = local_last_ - local_first_ + 1;
    const auto num_buffers = (remote_articles - local_articles) / 1000;
    return std::size_t(num_buffers) * 2;
}

void Update::Lock()
{
    state_->file_io_mutex.lock();
}

void Update::Unlock()
{
    state_->file_io_mutex.unlock();
}

void Update::SetProgressCallback(const HeaderTask::OnProgress& callback)
{
    state_->callback = callback;
}

std::string Update::group() const
{
    return state_->group;
}

std::string Update::path() const
{
    return state_->folder;
}

std::uint64_t Update::num_local_articles() const
{
    const auto num_local = local_last_ - local_first_ + 1;
    return num_local;
}

std::uint64_t Update::num_remote_articles() const
{
    const auto num_remote = remote_last_ - remote_first_ + 1;
    return num_remote;
}

} // newsflash

