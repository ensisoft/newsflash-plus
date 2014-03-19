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
#include <string>
#include <fstream>
#include "unit_test_common.h"
#include "../listing.h"
#include "../buffer.h"

std::vector<std::string> read_file(const char* file)
{
    std::vector<std::string> ret;
    std::ifstream in(file);

    while(in.good())
    {
        std::string line;
        std::getline(in, line);
        if (!line.empty())
            ret.push_back(std::move(line));
    }
    return ret;

}

void unit_test_success()
{

    {

        delete_file("listing.txt");        

        const char* body = 
           "alt.binaries.pictures.graphics.3d   900 800 y\r\n"            
           "alt.binaries.movies.divx 321 123 y\r\n"                      
           "alt.binaries.sounds.mp3    85 80 n\r\n";                      

        newsflash::buffer buff(body);

        auto listing = newsflash::listing("listing.txt");

        listing.receive(std::move(buff), 0);
        listing.finalize();

        const auto& lines = read_file("listing.txt");
        BOOST_REQUIRE(lines.size() == 4);
        BOOST_REQUIRE(lines[0] == "3");
        BOOST_REQUIRE(lines[1] == "alt.binaries.movies.divx,199");
        BOOST_REQUIRE(lines[2] == "alt.binaries.pictures.graphics.3d,101");
        BOOST_REQUIRE(lines[3] == "alt.binaries.sounds.mp3,6");

        delete_file("listing.txt");
    }

}

void unit_test_failure()
{
    
}

int test_main(int, char* [])
{

    unit_test_success();
    unit_test_failure();

    return 0;
}