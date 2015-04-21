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
#include <thread>
#include <future>
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
            min_pubdate_ = std::numeric_limits<std::time_t>::min();
            max_pubdate_ = std::numeric_limits<std::time_t>::max();
            min_size_    = std::numeric_limits<std::uint32_t>::min();
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
                auto mid = std::begin(items_) + size_;
                auto end = std::end(items_);
                std::reverse(beg, mid);
                std::reverse(mid, end);
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
            iterator beg;
            iterator end;
            iterator pos;
            if (!is_match(a))
            {
                beg = std::begin(items_) + size_;
                end = std::end(items_);
            }
            else
            {
                beg = std::begin(items_);
                end = std::begin(items_) + size_;
                ++size_;
            }
            switch (sorting_)
            {
                case sorting::sort_by_broken:
                    pos = lower_bound(beg, end, a, fileflag::broken); 
                    break;
                case sorting::sort_by_binary:
                    pos = lower_bound(beg, end, a, fileflag::binary);
                    break;
                case sorting::sort_by_downloaded:
                    pos = lower_bound(beg, end, a, fileflag::downloaded);
                    break;
                case sorting::sort_by_bookmarked:
                    pos = lower_bound(beg, end, a, fileflag::bookmarked);
                    break;

                case sorting::sort_by_date: 
                    pos = lower_bound(beg, end, a, &article_t::m_pubdate);
                    break;
                case sorting::sort_by_type: 
                    pos = lower_bound(beg, end, a, &article_t::m_type);
                    break;
                case sorting::sort_by_size:                    
                    pos = lower_bound(beg, end, a, &article_t::m_bytes); 
                    break;
                case sorting::sort_by_author:
                    pos = lower_bound(beg, end, a, &article_t::m_author);
                    break;
                case sorting::sort_by_subject:
                    pos = lower_bound(beg, end, a, &article_t::m_subject);
                    break;
            }
            items_.insert(pos, {key, index});
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
            auto pred = [this](const item& i) {
                const auto& a = on_load(i.key, i.index);
                return is_match(a);
            };

            if (size_ == items_.size())
            {
                auto it = std::stable_partition(std::begin(items_), std::end(items_), pred);
                size_ = std::distance(std::begin(items_), it);
                return;
            }

            std::deque<item> tmp;

            // partition both the currently visible items and the non-visible
            // items into visible and nonvisible. this partitions the whole
            // index into 4 sorted ranges like this.
            //
            // [aaaaaa|bbbbbbb|cccccccc|dddd]
            // beg    a   mid          b    end
            // 
            // where,
            // [beg, a) is the new visible range in previously visible range
            // [a, mid) is the new non-visible range in previously visible
            // [mid, b) is the new visible range in previously non-visible
            // [b, end) is the new non-visible range in previously non-visible
            // 
            // all the ranges are sorted according to the current sorting.
            // then we merge the 4 ranges into 2 sorted ranges such that,
            // [aaaaaaaa|bbbbbbbbbbb]
            // beg      mid         end
            //
            // where, 
            // [beg, mid) is the new visible range
            // [mid, end), is the new non-visible range.

            auto beg = std::begin(items_);
            auto mid = std::begin(items_) + size_;
            auto end = std::end(items_);

            auto future = std::async(std::launch::async, [&] {
                return std::stable_partition(mid, end, pred);
            });

            auto a = std::stable_partition(beg, mid, pred);            
            //auto b = std::stable_partition(mid, end, pred);                        

            future.wait();
            auto b = future.get();
           
            auto out = std::back_inserter(tmp);
            switch (sorting_)
            {
                case sorting::sort_by_broken:
                    merge(beg, a, mid, b, out, fileflag::broken);
                    merge(a, mid, b, end, out, fileflag::broken);
                    break;

                case sorting::sort_by_binary:
                    merge(beg, a, mid, b, out, fileflag::binary);
                    merge(a, mid, b, end, out, fileflag::binary);
                    break;

                case sorting::sort_by_downloaded:
                    merge(beg, a, mid, b, out, fileflag::downloaded);
                    merge(a, mid, b, end, out, fileflag::downloaded);
                    break;

                case sorting::sort_by_bookmarked:
                    merge(beg, a, mid, b, out, fileflag::bookmarked);
                    merge(a, mid, b, end, out, fileflag::bookmarked);
                    break;

                case sorting::sort_by_date:
                    merge(beg, a, mid, b, out, &article_t::m_pubdate);
                    merge(a, mid, b, end, out, &article_t::m_pubdate);
                    break;

                case sorting::sort_by_type:
                    merge(beg, a, mid, b, out, &article_t::m_type);
                    merge(a, mid, b, end, out, &article_t::m_type);
                    break; 

                case sorting::sort_by_size:
                    merge(beg, a, mid, b, out, &article_t::m_bytes);
                    merge(a, mid, b, end, out, &article_t::m_bytes);
                    break;

                case sorting::sort_by_author:
                    merge(beg, a, mid, b, out, &article_t::m_author);
                    merge(a, mid, b, end, out, &article_t::m_author);
                    break;
                    
                case sorting::sort_by_subject:
                    merge(beg, a, mid, b, out, &article_t::m_subject);
                    merge(a, mid, b, end, out, &article_t::m_subject);
                    break;
            }
            size_  = (a - beg) + (b - mid);
            items_ = std::move(tmp);
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

        using iterator = typename std::deque<item>::iterator;

        template<typename OutputIt, typename MemPtr>
        void merge(iterator first1, iterator last1,
            iterator first2, iterator last2, OutputIt out, MemPtr p)
        {
            if (sortdir_ == sortdir::ascending)
                std::merge(first1, last1, first2, last2, out, less(p));
            else std::merge(first1, last1, first2, last2, out, greater(p));
        }

        template<typename OutputIt>
        void merge(iterator first1, iterator last1,
            iterator first2, iterator last2, OutputIt out, fileflag mask)
        {
            if (sortdir_ == sortdir::ascending)
            {
                std::merge(first1, last1, first2, last2, out, [=](const item& lhs, const item& rhs) {
                    const auto& a = on_load(lhs.key, lhs.index);
                    const auto& b = on_load(rhs.key, rhs.index);
                    return (a.m_bits & mask).value()  < (b.m_bits & mask).value();
                });
            }
            else
            {
                std::merge(first1, last1, first2, last2, out, [=](const item& lhs, const item& rhs) {
                    const auto& a = on_load(lhs.key, lhs.index);
                    const auto& b = on_load(rhs.key, rhs.index);
                    return (a.m_bits & mask).value() > (b.m_bits & mask).value();
                });
            }
        }

        template<typename MemPtr>
        void sort(sortdir up_down, MemPtr p)
        {
            auto beg = std::begin(items_);
            auto mid = std::begin(items_) + size_;
            auto end = std::end(items_);
            if (up_down == sortdir::ascending) 
            {
                auto future = std::async(std::launch::async, [&] {
                    std::sort(beg, mid, less(p));
                });

                std::sort(mid, end, less(p));
                future.wait();
            }
            else 
            {
                auto future = std::async(std::launch::async, [&] {
                    std::sort(beg, mid, greater(p));
                });
                std::sort(mid, end, greater(p));                
                future.wait();
            }
        }

        void sort(sortdir up_down, fileflag mask)
        {
            auto beg = std::begin(items_);
            auto mid = std::begin(items_) + size_;
            auto end = std::end(items_);
            if (up_down == sortdir::ascending)
            {
                auto pred = [=](const item& lhs, const item& rhs) {
                    const auto& a = on_load(lhs.key, lhs.index);
                    const auto& b = on_load(rhs.key, rhs.index);
                    return (a.m_bits & mask).value()  < (b.m_bits & mask).value();
                };
                std::sort(beg, mid, pred);
                std::sort(mid, end, pred);
            }
            else
            {
                auto pred = [=](const item& lhs, const item& rhs) {
                    const auto& a = on_load(lhs.key, lhs.index);
                    const auto& b = on_load(rhs.key, rhs.index);
                    return (a.m_bits & mask).value() > (b.m_bits & mask).value();
                };
                std::sort(beg, mid, pred);
                std::sort(mid, end, pred);
            }
        }

        template<typename MemPtr>
        iterator lower_bound(iterator beg, iterator end, const article_t& a, MemPtr p)
        {
            if (sortdir_ == sortdir::ascending)
                return std::lower_bound(beg, end, a, less(p));
            else return std::lower_bound(beg, end, a, greater(p));
        }

        iterator lower_bound(iterator beg, iterator end, const article_t& a, fileflag mask)
        {
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
