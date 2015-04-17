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

#include <newsflash/config.h>
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

namespace newsflash
{

using catalog_t   = catalog<filebuf>;
using article_t   = article<filebuf>;
using idlist_t = idlist<filebuf>;

struct update::state {
    std::string folder;
    std::string group;

    //std::mutex m;
    std::map<std::uint32_t, std::unique_ptr<catalog_t>> files;
    std::map<std::uint32_t, std::uint32_t> hashmap;

    // message id db
    idlist_t idb;

    std::string file_volume_name(std::size_t index)
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
class update::parse : public action
{
public:
    parse(std::shared_ptr<state> s, buffer buff) : buffer_(std::move(buff))
    {}

    virtual void xperform() override
    {
        nntp::linebuffer lines(buffer_.content(), buffer_.content_length());
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

private:
    friend class update;
    std::shared_ptr<state> state_;
    std::vector<article_t> articles_;
private:
    buffer buffer_;
};

class update::store : public action
{
public:
    store(std::shared_ptr<state> s) : state_(s), first_(0), last_(0)
    {}

    virtual void xperform() override
    {
        first_ = std::numeric_limits<decltype(first_)>::max();

        auto& files = state_->files;
        auto& hmap  = state_->hashmap;
        auto& idb   = state_->idb;

        for (std::size_t i=0; i<articles_.size(); ++i)
        {
            auto& article = articles_[i];

            last_  = std::max(last_, article.number());
            first_ = std::min(first_, article.number());

            auto hit = hmap.find(article.hash());
            if (hit == std::end(hmap))
            {
                const auto index = article.number() / CATALOG_SIZE;
                hit = hmap.insert(std::make_pair(article.hash(), index)).first;
            }

            const auto file_index  = hit->second;
            const auto file_bucket = article.hash() % CATALOG_SIZE;            
            auto it = files.find(file_index);
            if (it == std::end(files))
            {
                std::unique_ptr<catalog_t> db(new catalog_t);
                db->open(state_->file_volume_name(file_index));
                it = files.insert(std::make_pair(file_index, std::move(db))).first;
            }
            auto& db = it->second;

            std::size_t slot;
            for (slot=0; slot<CATALOG_SIZE; ++slot)
            {
                const auto index = catalog_t::index_t((slot * 3 + file_bucket) % CATALOG_SIZE);

                if (!db->is_empty(index))
                {
                    auto a = db->load(index);
                    if (!a.is_match(article))
                        continue;

                    if (a.has_parts())
                    {
                        const auto max_parts = a.num_parts_total();
                        const auto num_part  = article.partno();
                        if (num_part > max_parts)
                            continue;

                        const auto base = a.number();
                        const auto num  = article.number();
                        std::int16_t diff = 0;
                        if (base > num)
                            diff = -(base - num);
                        else diff = num - base;
                        idb[a.idbkey() + num_part] = diff;
                    }
                    a.combine(article);
                    a.save();                    
                    break;
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
                    break;
                }
            }
            if (slot == CATALOG_SIZE)
                throw std::runtime_error("hashmap overflow");

            updates_.insert(db.get());
        }
        for (auto* db : updates_)
            db->flush();
    }
private:
    friend class update;
    std::shared_ptr<state> state_;
    std::vector<article_t> articles_;
    std::set<catalog_t*> updates_;
    std::uint64_t first_;
    std::uint64_t last_;    
private: 
    buffer buffer_;
};

update::update(std::string path, std::string group) : local_last_(0), local_first_(0)
{
    const auto nfo = fs::joinpath(fs::joinpath(path, group), group + ".nfo");
    const auto idb = fs::joinpath(fs::joinpath(path, group), group + ".idb");
    
    state_ = std::make_shared<state>();
    state_->folder = std::move(path);
    state_->group  = std::move(group);

#if defined(LINUX_OS)
    std::ifstream in(nfo, std::ios::in | std::ios::binary);
#elif defined(WINDOWS_OS)
    std::wifstream in(utf8::decode(nfo), std::ios::in);
#endif
    if (in.is_open())
    {
        in.seekg(0, std::ios::end);
        const unsigned long size = in.tellg();
        in.seekg(0, std::ios::beg);

        in.read((char*)&local_first_, sizeof(local_first_));
        in.read((char*)&local_last_, sizeof(local_last_));

        const auto num_items = (size - (sizeof(local_first_) + sizeof(local_last_))) / sizeof(uint32_t);
        std::vector<std::uint32_t> vec;
        vec.resize(num_items);
        in.read((char*)&vec[0], vec.size() * sizeof(std::uint32_t));
        for (std::size_t i=0; i<vec.size(); i+=2)
        {
            const auto key = vec[i];
            const auto val = vec[i+1];
            state_->hashmap.insert(std::make_pair(key, val));
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

update::~update()
{}

std::shared_ptr<cmdlist> update::create_commands() 
{
    std::shared_ptr<cmdlist> ret;

    if (remote_last_ == 0)
    {
        // get group information for retrieving the first and last
        // article numbers on the remote server.
        ret.reset(new cmdlist(cmdlist::groupinfo{state_->group}));
    }
    else if (xover_last_ < remote_last_)
    {
        cmdlist::overviews ov;
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
        ret.reset(new cmdlist(ov));
    }
    else if (xover_first_ > remote_first_)
    {
        cmdlist::overviews ov;
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
        ret.reset(new cmdlist(ov));
    }

    return std::move(ret);
}

void update::cancel()
{
    // the update is a little bit special.
    // we always want to retain the headers that've we grabbed
    // instead of having to wait untill the end of data before committing.
    commit();
}

void update::commit()
{
    const auto& path  = state_->folder;
    const auto& group = state_->group;
    const auto file = fs::joinpath(fs::joinpath(path, group), group + ".nfo");

    if (local_first_ == 0 || local_last_ == 0)
        return;

#if defined(LINUX_OS)
    std::ofstream out(file, std::ios::out | std::ios::binary | std::ios::trunc);
#elif defined(WINDOWS_OS)
    std::wofstream out(file, std::ios::out | std::ios::binary | std::ios::trunc);
#endif

    if (!out.is_open())
        throw std::runtime_error("unable to open: " + file);

    //out << local_first_ << std::endl;
    //out << local_last_  << std::endl;
    out.write((const char*)&local_first_, sizeof(local_first_));
    out.write((const char*)&local_last_, sizeof(local_last_));

    auto& files = state_->files;
    for (auto& p : files)
    {
        auto& db = p.second;
        db->flush();
    }

    std::vector<std::uint32_t> vec;
    const auto& hashmap = state_->hashmap;
    vec.reserve(hashmap.size() * 2);
    for (const auto& pair : hashmap)
    {
        vec.push_back(pair.first);
        vec.push_back(pair.second);
    }
    out.write((const char*)&vec[0], vec.size() * sizeof(std::uint32_t));
    out.close();
}

void update::complete(cmdlist& cmd, std::vector<std::unique_ptr<action>>& next)
{
    if (cmd.cmdtype() == cmdlist::type::groupinfo)
    {
        const auto& contents = cmd.get_buffers();
        const auto& buffer   = contents.at(0);
        const auto& pair = nntp::parse_group(buffer.content(), buffer.content_length());
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
        auto& contents = cmd.get_buffers();
        //auto& ranges   = cmd.get_commands();
        for (std::size_t i=0; i<contents.size(); ++i)
        {
            auto& content = contents[i];
            if (content.content_status() != buffer::status::success)
                continue;

            std::unique_ptr<action> p(new parse(state_, std::move(content)));
            p->set_affinity(action::affinity::any_thread);
            next.push_back(std::move(p));
        }
    }
}

void update::complete(action& a, std::vector<std::unique_ptr<action>>& next)
{
    if (auto* p = dynamic_cast<parse*>(&a))
    {
        std::unique_ptr<store> s(new store(state_));
        s->articles_ = std::move(p->articles_);
        s->buffer_ = std::move(p->buffer_);

        //s->set_affinity(action::affinity::single_thread);
        s->set_affinity(action::affinity::gui_thread);
        next.push_back(std::move(s));
    }
    if (auto* p = dynamic_cast<store*>(&a))
    {
        const auto first = p->first_;
        const auto last  = p->last_;
        local_first_ = std::min(local_first_, first);
        local_last_  = std::max(local_last_, last);

        if (on_info)
        {
            const auto num_remote = remote_last_ - remote_first_ + 1;
            const auto num_local  = local_last_ - local_first_ + 1;
            on_info(state_->group, num_local, num_remote);
        }

        if (on_write)
        {
            // remember that db might be accessed at the same time through
            // another thread via another store task
            for (auto* file : p->updates_)
            {
                on_write(file->filename());
            }
        }
    }
}

bool update::has_commands() const 
{
    if (remote_first_ == 0 && remote_last_  == 0)
        return true;
    return (remote_last_ != xover_last_) || (remote_first_ != xover_first_);
}

std::size_t update::max_num_actions() const 
{
    const auto remote_articles = remote_last_ - remote_first_ + 1;
    const auto local_articles = local_last_ - local_first_ + 1;
    const auto actions = (remote_articles - local_articles) / 1000;
    return actions * 2;
}

std::string update::group() const 
{
    return state_->group;
}

std::string update::path() const 
{
    return state_->folder;
}

std::uint64_t update::num_local_articles() const
{
    const auto num_local = local_last_ - local_first_ + 1;
    return num_local;    
}

std::uint64_t update::num_remote_articles() const
{
    const auto num_remote = remote_last_ - remote_first_ + 1;
    return num_remote;
}

} // newsflash
