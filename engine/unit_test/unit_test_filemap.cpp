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

#include "newsflash/warnpush.h"
#  include <boost/test/minimal.hpp>
#include "newsflash/warnpop.h"

#include <vector>
#include <algorithm>

#include "engine/filemap.h"
#include "unit_test_common.h"

std::size_t MB(double m)
{
    return m * 1024 * 1024;
}

std::size_t KB(double k)
{
    return k * 1024;
}

void test_create_open()
{
    delete_file("file");
    generate_file("file", MB(3));

    struct block {
        std::size_t size;
        unsigned char value;
    } blocks [] = {
        {KB(1), 0xaa},
        {KB(2.5), 0xbb},
        {KB(5), 0xcc},
        {MB(1), 0xdd},
        {MB(1.5), 0xee}
    };

    // create and write blocks of data
    {
        newsflash::filemap map;
        map.open("file");

        std::size_t offset = 0;

        for (auto b = std::begin(blocks); b != std::end(blocks); ++b)
        {
            auto r = map.load(offset, b->size, newsflash::filemap::buf_write);
            std::fill(r.begin(), r.end(), b->value);

            offset += b->size;
            std::cout << "wrote: " <<  b->size << " bytes" << std::endl;
            r.write();
        }
        map.flush();
    }

    // open existing
    {
        newsflash::filemap map;
        map.open("file");

        std::size_t offset = 0;

        for (auto b = std::begin(blocks); b != std::end(blocks); ++b)
        {
            auto r = map.load(offset, b->size, newsflash::filemap::buf_read);
            BOOST_REQUIRE(std::count(r.begin(), r.end(), b->value) == b->size);

            offset += b->size;
            std::cout << "read: " <<  b->size << " bytes" << std::endl;
        }
    }

    delete_file("file");
}

int test_main(int, char*[])
{
    test_create_open();
    return 0;
}