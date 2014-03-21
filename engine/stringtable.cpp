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

#include <cassert>
#include <cctype>
#include "stringtable.h"
#include "hash.h"

namespace engine
{

stringtable::stringtable()
{
#ifdef NEWSFLASH_DEBUG
    rawbytes_ = 0;
#endif
}


// add a new string to the stringtable. the algorithm is as follows.
// create a hash value for each string and see which bucket it belongs to.
// the first string in the bucket becomes the "base" string for the bucket.
// subsequent input strings that hash to the same bucket are only stored
// as difference to the base string (a sequence of delta (offset + change) values)
// if the string is the first string in the bucket it has 0 deltas
stringtable::key_t stringtable::add(const char* str, std::size_t len)
{
    assert(str);
    assert(len);

    len = std::min(len, std::size_t(MAX_STRING_LEN));

    engine::hash<char> hash;

    // add the length of the string to the hash also.
    // this way stuff such as 
    // "The_Magic_Stick_DVDRip.part10.rar (11/15)" gets 
    // a different base string than
    // "The_Magic_Stick_DVDrip.part10.rar (099/100)"
    hash.add(len);

    // create the hash and omit any digits
    for (std::size_t i=0; i<len; ++i)
    {
        if (!std::isdigit((unsigned char)str[i]))
            hash.add(str[i]);
    }

    const auto bucket = hash.result();

    // find where base string is stored for the bucket if any
    auto ret = find_base_string(bucket);
    if (!ret.first)
        ret = insert_base_string(bucket, str, len);

    const basestring* base   = ret.first;
    const std::size_t offset = ret.second;

    // 0 is not valid string len so we can -1 for a maximum total of 256 length
    diffstring s;
    s.offset = offset;
    s.length = len-1; // 0 is not valid so we can now store maximum 256 length string
    s.count = 0;

    // calculate delta to the base
    for (std::size_t i=0; i<len; ++i)
    {
        if (i > base->len+1 || base->data[i] != str[i])
        {
            delta d;
            d.pos = i;
            d.val = str[i];
            s.deltas[s.count++] = d;
        }
    }

    key_t key = diffdata_.size();

    // serialize into buffer
    const char* beg = static_cast<const char*>((const void*)&s);
    const char* end = beg + 6 + sizeof(delta) * s.count;
    std::copy(beg, end, std::back_inserter(diffdata_));

#ifdef NEWSFLASH_DEBUG
    rawbytes_ += len;
#endif

    return key;
}

std::string stringtable::get(key_t key) const
{
    const diffstring* s = load_diff_string(key);
    const basestring* b = load_base_string(s->offset);

    std::string ret(b->data, b->len+1);
    ret.resize(s->length+1);

    for (size_t i=0; i<s->count; ++i)
    {
        const delta& d = s->deltas[i];
        ret[d.pos] = d.val;
    }
    return ret;
}

#ifdef NEWSFLASH_DEBUG
    std::pair<std::size_t, std::size_t> stringtable::size_info() const
    {
        const auto tablesize = basedata_.size() + 
            diffdata_.size() + 
            offsets_.size() * sizeof(std::size_t) * 2;

        return { rawbytes_, tablesize };
    }
#endif

std::pair<stringtable::basestring*, std::size_t> stringtable::find_base_string(std::size_t bucket) const
{
    auto it = offsets_.find(bucket);
    if (it == offsets_.end())
        return {nullptr, 0};;

    // offset to the data array
    const auto offset = it->second;

    basestring* base = static_cast<basestring*>((void*)&basedata_[offset]);

    return {base, offset};

}

std::pair<stringtable::basestring*, std::size_t> stringtable::insert_base_string(std::size_t bucket, const char* str, std::size_t len)
{
    const auto offset = basedata_.size();

    basedata_.push_back(len-1);
    std::copy(str, str+len, std::back_inserter(basedata_));

    auto ret = offsets_.insert(std::make_pair(bucket, offset));
    assert(ret.second);

    basestring* base = static_cast<basestring*>((void*)&basedata_[offset]);

    return {base, offset};
}

const stringtable::diffstring* stringtable::load_diff_string(std::size_t offset) const
{
    return static_cast<const diffstring*>((const void*)&diffdata_[offset]);
}

const stringtable::basestring* stringtable::load_base_string(std::size_t offset) const
{
    return static_cast<const basestring*>((const void*)&basedata_[offset]);
}

} // engine
