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

#include <boost/test/minimal.hpp>
#include <vector>
#include <algorithm>
#include "../memory_mapped_file.h"
#include "../bigfile.h"
#include "unit_test_common.h"

std::size_t MB(int m)
{
    return m * 1024 * 1024;
}

std::size_t MB(double m)
{
    return m * 1024 * 1024;
}

std::size_t KB(int k)
{
    return k * 1024;
}

void test_map_whole_file()
{
    delete_file("file");

    // create test file with 3 disparate blocks of data
    std::vector<char> block;
    std::generate_n(std::back_inserter(block), MB(1), []() { return 0xaa; });
    std::generate_n(std::back_inserter(block), MB(2), []() { return 0xbb; });
    std::generate_n(std::back_inserter(block), MB(3), []() { return 0xcc; });

    block[0] = 0xda;
    block[block.size()-1] = 0xda;

    corelib::bigfile file;
    file.create("file");
    file.write(&block[0], block.size());
    file.flush();
    file.close();

    {
        corelib::memory_mapped_file map;
        BOOST_REQUIRE(map.map("file") == std::error_code());
        BOOST_REQUIRE(map.file_size() == block.size());

        char* data = (char*)map.data(0, block.size());
        BOOST_REQUIRE(!std::memcmp(data, &block[0], MB(1)));
        BOOST_REQUIRE(!std::memcmp(data + MB(1), &block[MB(1)], MB(1)));
        BOOST_REQUIRE(!std::memcmp(data + MB(3), &block[MB(3)], MB(3)));


        BOOST_REQUIRE(map.data(0, block.size()) == data);
        BOOST_REQUIRE(map.data(1, block.size()-1) == data + 1);
        BOOST_REQUIRE(map.data(2, block.size()-2) == data + 2);
        BOOST_REQUIRE(map.data(block.size()-1, 1) == data + block.size()-1);
        BOOST_REQUIRE(*(unsigned char*)map.data(0 ,1) == 0xda);
        BOOST_REQUIRE(*(unsigned char*)map.data(1, 1) == 0xaa);
        BOOST_REQUIRE(*(unsigned char*)map.data(block.size()-1, 1) == 0xda);

        BOOST_REQUIRE(map.map_count() == 1);

    }

    delete_file("file");
}

void test_map_chunk_file()
{
    delete_file("file");

    // create test file with 3 disparate blocks of data
    std::vector<char> data;
    std::generate_n(std::back_inserter(data), MB(1), []() { return 0xaa; });
    std::generate_n(std::back_inserter(data), MB(2), []() { return 0xbb; });
    std::generate_n(std::back_inserter(data), MB(3), []() { return 0xcc; });

    data[0] = 0xda;
    data[data.size()-1] = 0xda;

    corelib::bigfile file;
    file.create("file");
    file.write(&data[0], data.size());
    file.flush();
    file.close();

    // try various block sizes with a single map (map_count == 1)
    {
        struct block {
            std::size_t map_size;            
            std::size_t offset;
            std::size_t size;

        } blocks[] = {
            { KB(1), 0,     1 },
            { KB(1), 1,     1 },
            { KB(1), 0,     KB(1) },
            { KB(1), 512,   512 },
            { KB(1), KB(1), 1 },
            { KB(1), 768,   512 },
            { KB(1), 666,   30 },

            { KB(4), 0,     1 },
            { KB(4), 1,     1 },
            { KB(4), 0,     KB(4) },
            { KB(4), KB(2), KB(2) },
            { KB(4), KB(4), 1 },
            { KB(4), 3072,  KB(2) },
            { KB(4), 1234,  4000 },

            { MB(1), 0,       1 },
            { MB(1), 1,       1,},
            { MB(1), 0,       MB(1) },
            { MB(1), MB(0.5), MB(0.5) },
            { MB(1), MB(1),   1 },
            { MB(1), MB(0.5), MB(0.7)}

        };

        for (const block& block : blocks)
        {
            corelib::memory_mapped_file map;
            BOOST_REQUIRE(!map.map("file", block.map_size, 1));

            void* ptr = map.data(block.offset, block.size);
            BOOST_REQUIRE(!std::memcmp(ptr, &data[block.offset], block.size));
        }
    }

    // check multiple blocks
    {
        corelib::memory_mapped_file map;
        BOOST_REQUIRE(!map.map("file", KB(4), 2));

        map.data(0, 1);
        BOOST_REQUIRE(map.map_count() == 1);
        map.data(1, 1);
        BOOST_REQUIRE(map.map_count() == 1);

        map.data(KB(4) + 1, 1);
        BOOST_REQUIRE(map.map_count() == 2);

        map.data(KB(8) + 1, 1);
        BOOST_REQUIRE(map.map_count() == 2);

        map.data(0, 1);
        BOOST_REQUIRE(map.map_count() == 2);
    }

    delete_file("file");    
}

int test_main(int, char*[])
{
    test_map_whole_file();
    test_map_chunk_file();

    return 0;
}