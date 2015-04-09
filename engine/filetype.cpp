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

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <boost/regex.hpp>
#include <newsflash/warnpop.h>
#include <sstream>
#include "filetype.h"

namespace newsflash
{

std::vector<std::string> split(const std::string& input)
{
    std::stringstream ss(input);
    std::string s;
    std::vector<std::string> ret;
    while (std::getline(ss, s, '|'))
    {
        if (s[0] == ' ')
            s.erase(s.begin());
        if (s.back() == ' ')
            s.pop_back();
        ret.push_back(s);
    }
    return ret;
}

filetype find_filetype(const std::string& subject)
{
    struct pattern {
        filetype type;        
        const char* pattern;
    } patterns[] = {
        { filetype::audio, ".mp3 | .mp2 | .wav | .xm | .flac | .m3u | .mpa" },
        { filetype::video, ".avi | .mkv | .ogm | .wmv | .wma | .mpe?g | .rm | .mov | .flv | .asf | .mp4 | .3gp | .3g2 | .m4v" },
        { filetype::image, ".jpe?g | .bmp | .png | .gif" },
        { filetype::text,  ".txt | .nfo | .sfv | .log" },
        { filetype::archive, ".zip | .rar | .7z | .r\\d{1,3} | .\\d{2} | .part\\d{1,3}\\.rar" },
        { filetype::parity, ".par | .par2" },
        { filetype::document, ".doc | .chm | .pdf" }
    };

    for (auto it = std::begin(patterns); it != std::end(patterns); ++it)
    {
        const auto pat  = split((*it).pattern);
        const auto type = (*it).type;
        for (auto it = std::begin(pat); it != std::end(pat); ++it)
        {
            boost::regex r(*it, boost::regbase::icase /* | boost::regbase::perl */);
            if (boost::regex_search(subject.begin(), subject.end(), r))
                return type;
        }
    }
    return filetype::other;
}

} // newsflash