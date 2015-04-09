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

// $Id: string.h,v 1.7 2009/11/15 14:56:36 svaisane Exp $

#pragma once

#include <boost/range.hpp>
#include <algorithm>
#include <string>
#include <map>
#include <cassert>
#include <cctype>

namespace stringlib
{
    namespace detail
    {
        // Store distances between character values
        // within a string. This is a simple table based lookup
        // so char is assumed as the key type.
        template<typename DistanceType = size_t>
        class ascii_table_lookup
        {
        protected:
            ascii_table_lookup()
            {
                std::fill(boost::begin(table_), boost::end(table_), DistanceType());
            }
           ~ascii_table_lookup() {}

            const DistanceType& operator[](int i) const
            {
                assert(i < 256);
                return *(&table_[static_cast<unsigned char>(i)]);
            }
            DistanceType& operator[](int i)
            {
                assert(i < 256);
                return *(&table_[static_cast<unsigned char>(i)]);
            }
        private:
            DistanceType table_[256];
        };

        // store Distances between different objects 
        // within a range. DistanceType is a type that can be
        // used to measure and express the distance in a range
        // between objects of KeyType type.
        template<typename DistanceType, typename KeyType>
        class value_map_lookup
        {
        protected:
            value_map_lookup()
            {
            }
           ~value_map_lookup()
            {
            }
            const DistanceType& operator[](const KeyType& key) const
            {
                return table_[key];
            }
            DistanceType& operator[](const KeyType& key)
            {
                return table_[key];
            }
        private:
            typedef std::map<KeyType, DistanceType> map_type;
            map_type table_;
        };
        
        struct object_matcher
        {
            template<typename T1, typename T2> inline
            bool is_equal(const T1& lhs, const T2& rhs) const
            {
                return lhs == rhs;
            }
            template<typename T> inline
            T identity(const T& val) const
            {
                return val;
            }
        };

        // boyer-moore string search implementation
        template<typename Range, typename SkipTable>
        struct boyer_moore : public SkipTable
        {
        public:
            // boost.range may not necessarily be a copyable object
            // thus it is adviced to take a range object by (const) reference

            // create and preprocess a new "needle" object from the given range
            boyer_moore(const Range& needle) : needle_(needle)
            {
            }
            template<typename ObjectMatcher>
            boyer_moore(const Range& needle, const ObjectMatcher& om) : needle_(needle)
            {
            }

            template<typename ObjectMatcher>
            void preprocess(const ObjectMatcher& om)
            {
                if (boost::empty(needle_))
                    return;
                
                needle_iterator beg(boost::const_begin(needle_));
                needle_iterator end(boost::const_end(needle_));
                for (--end; beg != end; ++beg)
                    SkipTable::operator[](om.identity(*beg)) = std::distance(beg, end);
            }
            void preprocess()
            {
                preprocess(object_matcher());
            }

            inline bool empty() const
            {
                return boost::empty(needle_);
            }
            
            // this is an overload that defaults to using default object matching policy
            template<typename HaystackRange> inline
            bool is_within(const HaystackRange& haystack) const
            {
                return is_within(haystack, object_matcher());
            }


            // scan the haystack and see if the "this" needle  is within it.
            template<typename HaystackRange, typename ObjectMatcher>
            bool is_within(const HaystackRange& haystack, const ObjectMatcher& matcher) const
            {
                typedef typename boost::range_iterator<const HaystackRange>::type haystack_iterator;
                typedef typename boost::range_value<const HaystackRange>::type haystack_value;
                typedef typename boost::range_difference<const HaystackRange>::type haystack_difference;
                typedef typename boost::range_difference<const Range>::type difference;

                // simple sanity checking
                if (boost::empty(haystack) || boost::empty(needle_))
                    return false;

                difference length = boost::distance(needle_);
                if (boost::distance(haystack) < length)
                    return false; // haystack is too short, needle cannot match

                haystack_iterator begin(boost::const_begin(haystack));

                // scan loop
                while (true)
                {
                    haystack_iterator search(begin);
                    // start matching the needle and haystack backwards
                    
                    std::advance(search, length-1);
                    if (search >= boost::end(haystack))
                        return false;

                    assert(search < boost::const_end(haystack));

                    difference advance = difference();

                    needle_iterator pattern(boost::end(needle_));
                    // start matching from the last character
                    --pattern;
                    if (matcher.is_equal(*pattern, *search))
                    {
                        // match loop
                        while (matcher.is_equal(*pattern, *search))
                        {
                            // matched the whole needle, w00t
                            if (pattern == boost::begin(needle_))
                                return true;
                            --pattern, --search;

                            assert(search >= boost::begin(haystack));
                        }
                        // how many did we match? 
                        difference matched(std::distance(pattern, boost::end(needle_))-1);
                        advance  = SkipTable::operator[](matcher.identity(*search));
                        if (advance == difference())
                        {
                            // the mismatching character is not in the pattern
                            // shift the whole pattern past the mismatch
                            advance = length - matched;
                        }
                        else
                        {
                            if (matched < advance)
                                advance -= matched;
                        }
                    }
                    else
                    {
                        // there was no match. check if the mismatched character is
                        // within the needle. If it is not shift the needle completely
                        // past the mismatching character. Otherwise align the mismatching
                        // character with the first occurrence of the same character
                        // in the needle and match again.
                        advance = SkipTable::operator[](matcher.identity(*search));
                        if (advance == difference())
                            advance = length;
                    }
                    std::advance(begin, advance);
                }
                return false;
            }
        private:
            typedef typename
              boost::range_iterator<const Range>::type needle_iterator;
            const Range& needle_;
        };

    } // detail
    
    // std_locale simply wraps std::locale and implements 
    // the ObjectMatcher interface required by boyer-moore template
    class std_locale
    {
    public:
        // default construct to use current global locale object
        std_locale(const std::locale& loc = std::locale()) : locale_(loc) {}

        // compare to characters for equality by converting
        // them both to upper case before comparing
        inline
        bool is_equal(char c1, char c2) const
        {
            #ifdef _MSC_VER
                // what the *** there's a hickup with std::toupper 
                // taking the locale with vc7.1
                return std::toupper(c1) == std::toupper(c2);
                /*
                  return 
                  _totupper_l(c1, locale_) == 
                  _totupper_l(c2, locale_);
                */
            #else
                return 
                  std::toupper<char>(c1, locale_) == 
                  std::toupper<char>(c2, locale_);
            #endif
        }
        inline
        char identity(char c) const
        {
            #ifdef _MSC_VER
                return std::toupper(c);
            #else
                return std::toupper(c, locale_);
            #endif
        }
    private:
        std::locale locale_;
    };

    // a simple helper class that initializes a needle object
    // from a std::string and allows it to be reused over several searches
    template<typename Locale = std_locale>
    class string_matcher
    {
    public:
        // construct the needle from a string
        string_matcher(const std::string& needle, bool case_sensitive = true, const Locale& loc = Locale()) : 
          needle_string_(needle),
          needle_(needle_string_),
          locale_(loc),
          case_sensitive_(case_sensitive)
        {
            case_sensitive ? 
              needle_.preprocess() :
              needle_.preprocess(locale_) ;
        }
        // returns  true if needle is an empty string
        inline bool empty() const
        {
            return needle_.empty();
        }
        
        // search the haystack for this needle string.
        bool search(const std::string& haystack) const
        {
            return case_sensitive_ ? 
              needle_.is_within(haystack) :
              needle_.is_within(haystack, locale_);
        }

        bool search(const char* haystack) const
        {
            assert(haystack);
            return search(haystack, strlen(haystack));
        }
        bool search(const char* haystack, size_t len) const
        {
            assert(haystack);
            assert(len);
            typedef std::pair<const char*, const char*> range;
            range r(haystack, haystack + len);
            
            return case_sensitive_ ? 
              needle_.is_within(r) : 
              needle_.is_within(r, locale_);
        }
      
    private:
        typedef detail::boyer_moore<std::string, detail::ascii_table_lookup<> > matcher;
        std::string needle_string_;
        matcher needle_;
        Locale locale_;
        bool case_sensitive_;
    };
    

    template<typename Range> inline
    bool find_match(const Range& haystack, const Range& needle)
    {
        // TODO:
        /*
        using namespace detail;
        typedef typename 
          boost::range_value<Range>::type value_type;
        typedef typename
          boost::range_difference<Range>::type distance_type;
        typedef boyer_moore<Range, value_map_lookup<distance_type, value_type> > matcher;

        matcher m(needle);
        return m.is_within(haystack);
        */
        return false;
    }

    inline
    bool find_match(const std::string& haystack, const std::string& needle)
    {
        using namespace detail;
        typedef 
          boost::range_difference<std::string>::type distance_type;
        typedef boyer_moore<std::string, ascii_table_lookup<distance_type> > matcher;
        matcher m(needle);
        m.preprocess();
        return m.is_within(haystack);
    }

    // convenicen function for narrow character strings
    inline
    bool find_match(const char* haystack, size_t haystack_len,
                    const char* needle,   size_t needle_len)
    {
        if (!haystack || !needle)
            return false;
        using namespace detail;
        typedef std::pair<const char*, const char*> range;
        typedef boyer_moore<range, ascii_table_lookup<size_t> > matcher;
        
        range r_haystack(haystack, haystack + haystack_len);
        range r_needle(needle, needle + needle_len);
        
        matcher m(r_needle);
        m.preprocess();
        return m.is_within(r_haystack);
    }

    inline
    bool find_match(const char* haystack, const char* needle)
    {
        if (!haystack || !needle)
            return false;
        return find_match(haystack, strlen(haystack), needle, strlen(needle));
    }
    
    std::string find_longest_common_substring(std::vector<std::string> vec, bool case_sensitive = true);    
    
    // try to find the longest common substring in strings a and b
    std::string find_longest_common_substring(const std::string& a, const std::string& b, bool case_sensitive = true);

} // stringlib

