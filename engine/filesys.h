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

#include <string>

// file system functions

namespace fs
{

std::string remove_illegal_filename_chars(std::string name);

std::string remove_illegal_filepath_chars(std::string path);

std::string filename(int attempt, const std::string& name);

// join i.e. concatenate b to to the end of path a
// for example joinpath("/foo/bar", "keke.derp") -> "/foo/bar/keke.derp"
std::string joinpath(const std::string& path, const std::string& str);

// create the path recursively with all the directories in the path.
std::string createpath(const std::string& path);

bool exists(const std::string& path);

bool is_illegal_filepath_char(int c);
bool is_illegal_filename_char(int c);


} // fs
