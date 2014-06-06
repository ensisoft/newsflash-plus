// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
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

#include <newsflash/config.h>

#include <newsflash/sdk/format.h>

#include <cstring>
#include "cmdline.h"

namespace {
    QString     g_executable_path;
    QStringList g_cmd_line;
} // namespace


namespace gui
{

QString get_installation_directory()
{
#if defined(WINDOWS_OS)
    QString s;
    // Using the first argument of the executable doesnt work with Unicode
    // paths, but the system mangles the path
    std::wstring buff;
    buff.resize(1024);

    DWORD ret = GetModuleFileName(NULL, &buff[0], buff.size());
    if (ret == 0)
        return "";
    
    s = widen(buff);
    int i = s.lastIndexOf("\\");
    if (i ==-1)
        return "";
    s = s.left(i+1);
    return s;

#elif defined(LINUX_OS)

    return g_executable_path;

#endif

}


void set_cmd_line(int argc, char* argv[])
{
#if defined(WINDOWS_OS)
    int num_cmd_args = 0;
    LPWSTR* cmds = CommandLineToArgW(GetCommandLineW(),
        &num_cmd_args);

    for (int i=0; i<num_cmd_args)
    {
        g_cmd_line.push_back(widen(cmds[i]));
    }
    LocalFree(cmds);

#elif defined(LINUX_OS)
    std::string tmp;
    auto e = argv[0];
    auto i = std::strlen(argv[0]);
    for (i=i-1; i>0; --i)
    {
        if (e[i] == '/')
        {
            tmp.append(e, i+1);
            break;
        }
    }
    g_executable_path = sdk::widen(tmp.c_str());

    for (int i=0; i<argc; ++i)
    {
        g_cmd_line.push_back(sdk::widen(argv[0]));
    }

#endif
}

QStringList get_cmd_line()
{
    return g_cmd_line;
}

} // gui
