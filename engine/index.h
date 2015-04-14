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
#include <functional>
#include <algorithm>
#include <deque>
#include <limits>
#include <cstdint>
#include <cstring>
#include <cassert>
#include "article.h"
#include "assert.h"
#include "bitflag.h"

namespace newsflash
{
    // index for ordered article access.
    template<typename Storage>
    class index
    {
        struct item;
    public:
        enum class sorting {
            sort_by_broken,
            sort_by_binary,
            sort_by_downloaded,
            sort_by_bookmarked,
            sort_by_date, 
            sort_by_type,
            sort_by_size,
            sort_by_author,
            sort_by_subject,
        };
        enum class sortdir {
            ascending, descending
        };

        enum class flags {
            selected
        };

        using article_t = typename newsflash::article<Storage>;        

        index() : size_(0), sorting_(sorting::sort_by_date), sortdir_(sortdir::ascending)
        {
            types_.set_from_value(~0);
            flags_.set_from_value(~0);
            min_pubdate_ = 0;
            max_pubdate_ = std::numeric_limits<std::time_t>::max();
            min_size_    = 0;
            max_size_    = std::numeric_limits<std::uint32_t>::max();
        }

        // callback to invoke to load an article.
        using loader = std::function<article_t (std::size_t key, std::size_t index)>;

        loader on_load;

        void sort(sorting column, sortdir up_down)
        {
            if (column == sorting_)
            {
                auto beg = std::begin(items_);
                auto end = std::begin(items_);
                std::advance(end, size_);
                std::reverse(beg, end);
                sorting_ = column;
                sortdir_ = up_down;
                return;
            }
            sorting_ = column;
            sortdir_ = up_down;            
            resort();
        }
        void resort()
        {
            switch (sorting_)
            {
                case sorting::sort_by_broken:     
                    sort(sortdir_, fileflag::broken); 
                    break;
                case sorting::sort_by_binary:     
                    sort(sortdir_, fileflag::binary); 
                    break;
                case sorting::sort_by_downloaded: 
                    sort(sortdir_, fileflag::downloaded); 
                    break;
                case sorting::sort_by_bookmarked: 
                    sort(sortdir_, fileflag::bookmarked); 
                    break;
                case sorting::sort_by_date:        
                    sort(sortdir_, &article_t::m_pubdate); 
                    break;
                case sorting::sort_by_type:       
                    sort(sortdir_, &article_t::m_type); 
                    break;
                case sorting::sort_by_size:       
                    sort(sortdir_, &article_t::m_bytes); 
                    break;
                case sorting::sort_by_author:     
                    sort(sortdir_, &article_t::m_author); 
                    break;
                case sorting::sort_by_subject:    
                    sort(sortdir_, &article_t::m_subject); 
                    break;
            }            
        }

        // insert the new item into the index in the right position.
        // returns the position which is given to the inserted item.
        // this will maintain current sorting.
        void insert(const article_t& a, std::size_t key, std::size_t index)
        {
            if (!is_match(a))
            {
                items_.push_back({key, index});
                return;
            }

            typename std::deque<item>::iterator it;
            switch (sorting_)
            {
                case sorting::sort_by_broken:
                    it = lower_bound(a, fileflag::broken); 
                    break;
                case sorting::sort_by_binary:
                    it = lower_bound(a, fileflag::binary);
                    break;
                case sorting::sort_by_downloaded:
                    it = lower_bound(a, fileflag::downloaded);
                    break;
                case sorting::sort_by_bookmarked:
                    it = lower_bound(a, fileflag::bookmarked);
                    break;

                case sorting::sort_by_date: 
                    it = lower_bound(a, &article_t::m_pubdate);
                    break;
                case sorting::sort_by_type: 
                    it = lower_bound(a, &article_t::m_type);
                    break;
                case sorting::sort_by_size:                    
                    it = lower_bound(a, &article_t::m_bytes); 
                    break;
                case sorting::sort_by_author:
                    it = lower_bound(a, &article_t::m_author);
                    break;
                case sorting::sort_by_subject:
                    it = lower_bound(a, &article_t::m_subject);
                    break;
            }
            items_.insert(it, {key, index});
            ++size_;
        }

        std::size_t size() const 
        { 
            return size_;
        }

        std::size_t real_size() const 
        {
            return items_.size();
        }

        article_t operator[](std::size_t index) const
        {
            assert(index < size_);
            const auto& item = items_[index];
            return on_load(item.key, item.index);
        }

        sorting get_sorting() const 
        { return sorting_; }

        sortdir get_sortdir() const 
        { return sortdir_; }

        void select(std::size_t index, bool val) 
        {
            assert(index < size_);
            auto& item = items_[index];
            item.bits.set(flags::selected, val);
        }

        bool is_selected(std::size_t index) const 
        {
            assert(index < size_);
            const auto& item = items_[index];
            return item.bits.test(flags::selected);
        }

        bool show_deleted() const 
        {
            return flags_.test(fileflag::deleted);
        }

        // filter the index by displaying only articles with matching file types
        void set_type_filter(filetype type, bool on_off)
        {
            types_.set(type, on_off);
        }

        // filter the index by displaying only articles with matching filetype
        void set_type_filter(bitflag<filetype> types)
        {
            types_ = types;
        }

        void set_flag_filter(bitflag<fileflag> flags)
        {
            flags_ = flags;
        }
        void set_flag_filter(fileflag flag, bool on_off)
        {
            flags_.set(flag, on_off);
        }

        void set_date_filter(std::time_t min_pubdate, std::time_t max_pubdate)
        {
            assert(max_pubdate >= min_pubdate);
            min_pubdate_ = min_pubdate;
            max_pubdate_ = max_pubdate;
        }

        void set_size_filter(std::uint64_t min_size, std::uint64_t max_size)
        {
            assert(max_size >= min_size);
            min_size_ = min_size;
            max_size_ = max_size;
        }

        void filter()
        {
            // we might have items from previous filter that are currently
            // not being displayed. since the filter might become
            // more "relaxed" and those items might match we need to put them 
            // back into the "visible" range. also note that we must maintain
            // the correct sorting

            std::deque<item> maybe;
            std::copy(std::begin(items_) + size_, std::end(items_),
                std::back_inserter(maybe));

            items_.resize(size_);

            auto beg = std::begin(items_);
            auto end = std::begin(items_);
            std::advance(end, size_);
            auto it = std::stable_partition(beg, end, [&](const item& i) {
                const auto& a = on_load(i.key, i.index);
                return is_match(a);
            });
            size_ = std::distance(std::begin(items_), it);            

            for (const auto& i : maybe)
            {
                const auto& article = on_load(i.key, i.index);
                insert(article, i.key, i.index);
            }            
        }

    private:
        template<typename Class, typename Member>
        struct bigger_t {
            typedef Member (Class::*MemPtr);

            bigger_t(MemPtr p, loader func) : ptr_(p), load_(func)
            {}

            bool operator()(const item& lhs, const item& rhs) const 
            {
                const auto& a = load_(lhs.key, lhs.index);
                const auto& b = load_(rhs.key, rhs.index);
                return a.*ptr_ > b.*ptr_;
            }
            bool operator()(const item& lhs, const article_t& rhs) const 
            {
                const auto& a = load_(lhs.key, lhs.index);
                const auto& b = rhs;
                return a.*ptr_ > b.*ptr_;
            }
        private:
            MemPtr ptr_;
            loader load_;
        };
        template<typename Class, typename Member>
        struct smaller_t {
            typedef Member (Class::*MemPtr);

            smaller_t(MemPtr p, loader func) : ptr_(p), load_(func)
            {}

            bool operator()(const item& lhs, const item& rhs) const 
            {
                const auto& a = load_(lhs.key, lhs.index);
                const auto& b = load_(rhs.key, rhs.index);
                return a.*ptr_ < b.*ptr_;
            }
            bool operator()(const item& lhs, const article_t& rhs) const 
            {
                const auto& a = load_(lhs.key, lhs.index);
                const auto& b = rhs;
                return a.*ptr_ < b.*ptr_;
            }
        private:
            MemPtr ptr_;
            loader load_;
        };

        template<typename Class, typename Member>
        smaller_t<Class, Member> less(Member Class::*p)
        {
            return smaller_t<Class, Member>(p, on_load);
        }
        template<typename Class, typename Member>
        bigger_t<Class, Member> greater(Member Class::*p)
        {
            return bigger_t<Class, Member>(p, on_load);
        }

        template<typename MemPtr>
        void sort(sortdir up_down, MemPtr p)
        {
            auto beg = std::begin(items_);
            auto end = std::begin(items_);
            std::advance(end, size_);
            if (up_down == sortdir::ascending) 
                 std::sort(beg, end, less(p));
            else std::sort(beg, end, greater(p));
        }
        void sort(sortdir up_down, fileflag mask)
        {
            auto beg = std::begin(items_);
            auto end = std::begin(items_);
            std::advance(end, size_);
            if (up_down == sortdir::ascending)
            {
                std::sort(beg, end, [=](const item& lhs, const item& rhs) {
                    const auto& a = on_load(lhs.key, lhs.index);
                    const auto& b = on_load(rhs.key, rhs.index);
                    return (a.m_bits & mask).value()  < (b.m_bits & mask).value();
                });
            }
            else
            {
                std::sort(beg, end, [=](const item& lhs, const item& rhs) {
                    const auto& a = on_load(lhs.key, lhs.index);
                    const auto& b = on_load(rhs.key, rhs.index);
                    return (a.m_bits & mask).value() > (b.m_bits & mask).value();
                });
            }
        }

        template<typename MemPtr>
        typename std::deque<item>::iterator lower_bound(const article_t& a, MemPtr p)
        {
            auto beg = std::begin(items_);
            auto end = std::begin(items_);
            std::advance(end, size_);
            if (sortdir_ == sortdir::ascending)
                return std::lower_bound(beg, end, a, less(p));
            else return std::lower_bound(beg, end, a, greater(p));
        }

        typename std::deque<item>::iterator lower_bound(const article_t& a, fileflag mask)
        {
            auto beg = std::begin(items_);
            auto end = std::begin(items_);
            std::advance(end, size_);
            if (sortdir_ == sortdir::ascending) {
                return std::lower_bound(beg, end, a, [=](const item& lhs, const article_t& rhs) {
                    const auto& a = on_load(lhs.key, lhs.index);
                    const auto& b = rhs;
                    return (a.m_bits & mask).value() < (b.m_bits & mask).value();
                });
            }
            else
            {
                return std::lower_bound(beg, end, a, [=](const item& lhs, const article_t& rhs) {
                    const auto& a = on_load(lhs.key, lhs.index);
                    const auto& b = rhs;
                    return (a.m_bits & mask).value() > (b.m_bits & mask).value();
                });
            }
        }

        bool is_match(const article_t& a) const 
        {
            if (!types_.test(a.type()))
                return false;

            if (!flags_.test(fileflag::broken) && 
                a.m_bits.test(fileflag::broken))
                return false;

            if (!flags_.test(fileflag::deleted) &&
                a.m_bits.test(fileflag::deleted))
                return false;

            const auto date = a.pubdate();
            if (date < min_pubdate_ || date > max_pubdate_)
                return false;

            const auto size = a.bytes();
            if (size < min_size_ || size > max_size_)
                return false;

            return true;
        }

    private:
        struct item {
            std::size_t key;
            std::size_t index;
            bitflag<flags> bits;
        };
        std::deque<item> items_;
        std::size_t size_;
    private:
        sorting sorting_;
        sortdir sortdir_;
    private:
        bitflag<filetype, std::uint16_t> types_;
        bitflag<fileflag, std::uint16_t> flags_;
        std::time_t min_pubdate_;
        std::time_t max_pubdate_;
        std::uint64_t min_size_;
        std::uint64_t max_size_;
    };

} // newsflash
