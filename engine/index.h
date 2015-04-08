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
#include <cstdint>
#include <cstring>
#include <cassert>
#include "article.h"
#include "assert.h"

namespace newsflash
{
    // index for ordered article access.
    class index
    {
        struct item;
    public:
        enum class sorting {
            sort_by_broken,
            sort_by_binary,
            sort_by_downloaded,
            sort_by_bookmarked,
            sort_by_age, 
            sort_by_type,
            sort_by_size,
            sort_by_author,
            sort_by_subject,
        };
        enum class filter {

        };

        enum class order {
            ascending, descending
        };

        index() : size_(0), sort_(sorting::sort_by_age), order_(order::ascending)
        {}

        // callback to invoke to load an article.
        using loader = std::function<article (std::size_t key, std::size_t index)>;

        loader on_load;

        void sort(sorting column, order up_down)
        {
            if ((column == sort_ ) && (up_down == order_))
                return;
            if (column == sort_)
            {
                auto beg = std::begin(items_);
                auto end = std::begin(items_);
                std::advance(end, size_);
                std::reverse(beg, end);
                sort_  = column;
                order_ = up_down;
                return;
            }

            switch (column)
            {
                case sorting::sort_by_broken:     sort(up_down, article::flags::broken); break;
                case sorting::sort_by_binary:     sort(up_down, article::flags::binary); break;
                case sorting::sort_by_downloaded: sort(up_down, article::flags::downloaded); break;
                case sorting::sort_by_bookmarked: sort(up_down, article::flags::bookmarked); break;
                case sorting::sort_by_age:        sort(up_down, &article::pubdate); break;
                case sorting::sort_by_type:       sort(up_down, &article::type); break;
                case sorting::sort_by_size:       sort(up_down, &article::bytes); break;
                case sorting::sort_by_author:     sort(up_down, &article::author); break;
                case sorting::sort_by_subject:    sort(up_down, &article::subject); break;
            }
            sort_  = column;
            order_ = up_down;            
        }
        // insert the new item into the index in the right position.
        // returns the position which is given to the inserted item.
        // this will maintain current sorting.
        std::size_t insert(const article& a, std::size_t key, std::size_t index)
        {
            std::deque<item>::iterator it;
            switch (sort_)
            {
                case sorting::sort_by_broken:
                    it = lower_bound(a, article::flags::broken); 
                    break;
                case sorting::sort_by_binary:
                    it = lower_bound(a, article::flags::binary);
                    break;
                case sorting::sort_by_downloaded:
                    it = lower_bound(a, article::flags::downloaded);
                    break;
                case sorting::sort_by_bookmarked:
                    it = lower_bound(a, article::flags::bookmarked);
                    break;

                case sorting::sort_by_age: 
                    it = lower_bound(a, &article::pubdate);
                    break;
                case sorting::sort_by_type: 
                    it = lower_bound(a, &article::type);
                    break;
                case sorting::sort_by_size:                    
                    it = lower_bound(a, &article::bytes); 
                    break;
                case sorting::sort_by_author:
                    it = lower_bound(a, &article::author);
                    break;
                case sorting::sort_by_subject:
                    it = lower_bound(a, &article::subject);
                    break;
            }
            items_.insert(it, {key, index});

            ++size_;
            return std::distance(std::begin(items_), it);
        }

        std::size_t size() const 
        { 
            return size_;
        }

        article operator[](std::size_t index) const
        {
            assert(index < size_);
            const auto& item = items_[index];
            return on_load(item.key, item.index);
        }

        sorting get_sorting() const 
        { return sort_; }

        order get_order() const 
        { return order_; }

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
            bool operator()(const item& lhs, const article& rhs) const 
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
            bool operator()(const item& lhs, const article& rhs) const 
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
        void sort(order up_down, MemPtr p)
        {
            auto beg = std::begin(items_);
            auto end = std::begin(items_);
            std::advance(end, size_);
            if (up_down == order::ascending) 
                 std::sort(beg, end, less(p));
            else std::sort(beg, end, greater(p));
        }
        void sort(order up_down, article::flags mask)
        {
            auto beg = std::begin(items_);
            auto end = std::begin(items_);
            std::advance(end, size_);
            if (up_down == order::ascending)
            {
                std::sort(beg, end, [=](const item& lhs, const item& rhs) {
                    const auto& a = on_load(lhs.key, lhs.index);
                    const auto& b = on_load(rhs.key, rhs.index);
                    return (a.bits & mask).value()  < (b.bits & mask).value();
                });
            }
            else
            {
                std::sort(beg, end, [=](const item& lhs, const item& rhs) {
                    const auto& a = on_load(lhs.key, lhs.index);
                    const auto& b = on_load(rhs.key, rhs.index);
                    return (a.bits & mask).value() > (b.bits & mask).value();
                });
            }
        }

        template<typename MemPtr>
        std::deque<item>::iterator lower_bound(const article& a, MemPtr p)
        {
            auto beg = std::begin(items_);
            auto end = std::begin(items_);
            std::advance(end, size_);
            if (order_ == order::ascending)
                return std::lower_bound(beg, end, a, less(p));
            else return std::lower_bound(beg, end, a, greater(p));
        }

        std::deque<item>::iterator lower_bound(const article& a, article::flags mask)
        {
            auto beg = std::begin(items_);
            auto end = std::begin(items_);
            std::advance(end, size_);
            if (order_ == order::ascending) {
                return std::lower_bound(beg, end, a, [=](const item& lhs, const article& rhs) {
                    const auto& a = on_load(lhs.key, lhs.index);
                    const auto& b = rhs;
                    return (a.bits & mask).value() < (b.bits & mask).value();
                });
            }
            else
            {
                return std::lower_bound(beg, end, a, [=](const item& lhs, const article& rhs) {
                    const auto& a = on_load(lhs.key, lhs.index);
                    const auto& b = rhs;
                    return (a.bits & mask).value() > (b.bits & mask).value();
                });
            }
        }

        std::deque<item>::iterator end() 
        { 
            auto it = std::begin(items_);
            std::advance(it, size_);
            return it;
        }

    private:
        struct item {
            std::size_t key;
            std::size_t index;
        };
        std::deque<item> items_;
        std::size_t size_;
        sorting sort_;
        order   order_;
    };

} // newsflash
