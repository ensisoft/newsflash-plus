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

#pragma once

#include "newsflash/config.h"

#include <string>
#include <cstdint>

namespace newsflash
{
    enum class filetype : std::uint8_t
    {
        // no file type recognized.
        none,

        // audio file such as .mp3
        audio,

        // video file such as .mkv
        video,

        // image file such as .jpg or .png
        image,

        // text file such as .txt, .nfo
        text,

        // archive file such as .zip or .rar
        archive,

        // par2 parity file, .par2 or .vol000-001.par
        parity,

        // document such as .pdf
        document,

        // something else
        other
    };

    filetype find_filetype(const std::string& filename);

    enum class fileflag : std::uint8_t {
        broken,

        // the article appears to contain binary content.
        binary,

        // the article is deleted.
        deleted,

        // the article is downloaded.
        downloaded,

        // the article is bookmarked
        bookmarked,

        // the article string data is utf8 encoded.
        enable_utf8
    };

} // newsflash