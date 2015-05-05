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
#include <vector>
#include <cctype>
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

std::string tolower(const std::string& s)
{
    std::string ret;
    for (const auto& c : s)
        ret.push_back(std::tolower(c));

    return ret;
}

filetype find_filetype(const std::string& filename)
{
    struct pattern {
        filetype type;        
        const char* str;
    } patterns[] = {
        { filetype::audio, ".mp3 | .mp2 | .wav | .xm | .flac | .m3u | .pls | .mpa | .ogg" },
        { filetype::video, ".avi | .mkv | .ogm | .wmv | .wma | .mpe?g | .rm | .mov | .flv | .asf | .mp4 | .3gp | .3g2 | .m4v" },
        { filetype::image, ".jpe?g | .bmp | .png | .gif" },
        { filetype::text,  ".txt | .nfo | .sfv | .log | .nzb | .rtf | .cue"},
        { filetype::archive, ".zip | .rar | .7z | .r\\d{1,3} | .\\d{2} | .part\\d{1,3}\\.rar" },
        { filetype::parity, ".par | .par2" },
        { filetype::document, ".doc | .chm | .pdf" }
    };

    const auto pos = filename.find_last_of(".");
    if (pos == std::string::npos)
        return filetype::other;

    const auto xtension = tolower(filename.substr(pos));

    // note that the regular expressions above should be of the form \\.mp3 
    // i.e. we need to escape the . 

    for (auto it = std::begin(patterns); it != std::end(patterns); ++it)
    {
        const auto pat  = split((*it).str);
        const auto type = (*it).type;
        for (auto it = std::begin(pat); it != std::end(pat); ++it)
        {
            // escape the . and interpret it as a char
            boost::regex r("\\" + *it, boost::regbase::icase /* | boost::regbase::perl */);
            if (boost::regex_search(xtension.begin(), xtension.end(), r))
                return type;
        }
    }
    return filetype::other;
}

} // newsflash
