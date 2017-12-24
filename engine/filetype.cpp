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
#  include <boost/regex.hpp>
#include "newsflash/warnpop.h"

#include <sstream>
#include <iomanip>
#include <vector>
#include <cctype>
#include <map>

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
    // using std::tolower is very very slow. (profiled by valgrind)
    // we're spending considerable time in just calling tolower.
    //
    // Actually we don't need to call std::tolower
    // since we're only interested in a small subset of characters
    // that are in the ASCII set.
    // see the find_filetype function


    struct LowerCaseTable {
        unsigned char lower_cases[256];

        LowerCaseTable()
        {
            // initialize with identities.
            for (unsigned i=0; i<=255; ++i)
            {
                lower_cases[i] = i;
            }
            for (unsigned i='A'; i<= 'Z'; ++i)
            {
                lower_cases[i] = i + 0x20;
            }
        }
    };
    static LowerCaseTable table;

    std::string ret;
    ret.resize(s.size());
    for (size_t i=0; i<s.size(); ++i)
    {
        const char the_char = s[i];
        ret[i] = table.lower_cases[static_cast<unsigned char>(the_char)];
    }

#if 0
    for (const auto& c : s)
        ret.push_back(std::tolower(c));
#endif

    return ret;
}

struct FileTypePatterns{
    filetype type;
    const char* str;
} FileTypePatterns[] = {
    { filetype::audio,    ".mp3 | .mp2 | .wav | .xm | .flac | .m3u | .pls | .mpa | .ogg" },
    { filetype::video,    ".avi | .mkv | .ogm | .wmv | .wma | .mpeg | | .mpg | .rm | .mov | .flv | .asf | .mp4 | .3gp | .3g2 | .m4v" },
    { filetype::image,    ".jpg | .jpeg | .bmp | .png | .gif" },
    { filetype::text,     ".txt | .nfo | .sfv | .log | .nzb | .rtf | .cue"},
    { filetype::archive,  ".zip | .rar | .7z" },
    { filetype::parity,   ".par | .par2" },
    { filetype::document, ".doc | .chm | .pdf" }
};

filetype find_filetype(const std::string& filename)
{
    struct ExtensionFileTypeMap {
        ExtensionFileTypeMap()
        {
            for (auto it = std::begin(FileTypePatterns); it != std::end(FileTypePatterns); ++it)
            {
                const auto& pat = split((*it).str);
                const auto type = (*it).type;
                for (auto p : pat)
                {
                    typemap[p] = type;
                }
            }

            // some special cases.
            for (int i=0; i<100; ++i)
            {
                std::stringstream ss;
                ss << std::setfill('0') << std::setw(2) << i;
                std::string s;
                ss >> s;
                typemap[".r" + s] = filetype::archive;
                typemap["." + s] = filetype::archive;
            }

            for (int i=0; i<1000; ++i)
            {
                std::stringstream ss;
                ss << std::setfill('0') << std::setw(2) << i;
                std::string s;
                ss >> s;
                typemap[".r" + s] = filetype::archive;
                typemap[".part" + s] = filetype::archive;
            }

        }
        std::map<std::string, filetype> typemap;
    };

    static ExtensionFileTypeMap map;

    const auto pos = filename.find_last_of(".");
    if (pos == std::string::npos)
        return filetype::other;

    const auto& ext = tolower(filename.substr(pos));

    auto it = map.typemap.find(ext);
    if (it != std::end(map.typemap))
        return it->second;

#if 0
    // note that the regular expressions above should be of the form \\.mp3
    // i.e. we need to escape the
    for (auto it = std::begin(FileTypePatterns); it != std::end(FileTypePatterns); ++it)
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
#endif
    return filetype::other;
}

} // newsflash
