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
#include <newsflash/warnpush.h>
#  include <QFile>
#  include <QTextStream>
#  include <QIODevice>
#  include <QStringList>
#include <newsflash/warnpop.h>

#if defined(WINDOWS_OS)
#  include <windows.h>
#endif

#include "platform.h"

namespace app
{

#if defined(WINDOWS_OS)

QString get_platform_name()
{
    OSVERSIONINFOEX info = {};
    info.dwOSVersionInfoSize = sizeof(info);
    if (GetVersionEx(reinterpret_cast<OSVERSIONINFO*>(&info)) == FALSE)
        return "";

    SYSTEM_INFO sys = {};
    GetSystemInfo(&sys);

    QString ret;

    // http://msdn.microsoft.com/en-us/library/ms724833(v=VS.85).aspx
    // major versions
    if (info.dwMajorVersion == 6)
    {
        if (info.dwMinorVersion == 1)
        {
            if (info.wProductType == VER_NT_WORKSTATION)
                ret = "Windows 7";
            else ret = "Windows Server 2008 R2";
        }
        else if (info.dwMinorVersion == 0)
        {
            if (info.wProductType != VER_NT_WORKSTATION)
                ret = "Windows Server 2008";
            else ret = "Windows Vista";
        }
    }
    else if (info.dwMajorVersion == 5)
    {
        // MSDN says this API is available from Windows 2000 Professional
        // but the headers don't have the definition....
#ifndef VER_SUITE_WH_SERVER
        enum {
            VER_SUITE_WH_SERVER = 0x00008000
        };
#endif 
        if (info.dwMinorVersion == 2 && GetSystemMetrics(SM_SERVERR2))
            ret = "Windows Server 2003 R2";
        else if (info.dwMinorVersion == 2 && GetSystemMetrics(SM_SERVERR2) == 0)
            ret = "Windows Server 2003";
        else if (info.dwMinorVersion == 2 && info.wSuiteMask & VER_SUITE_WH_SERVER)
            ret = "Windows Home Server";
        else if (info.dwMinorVersion == 2 && info.wProductType == VER_NT_WORKSTATION  && sys.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
            ret = "Windows XP Professional x64 Edition";
        else if (info.dwMinorVersion == 1)
        {
            ret = "Windows XP";
            if (info.wSuiteMask & VER_SUITE_PERSONAL)
                ret += " Home Edition";
            else ret += " Professional";
        }
        else if (info.dwMinorVersion == 0)
        {
            ret = "Windows 2000";
            if (info.wProductType == VER_NT_WORKSTATION)
                ret += " Professional";
            else
            {
                if (info.wSuiteMask & VER_SUITE_DATACENTER)
                    ret += " Datacenter Server";
                else if (info.wSuiteMask & VER_SUITE_ENTERPRISE)
                    ret += " Advanced Server";
                else ret += " Server";
            }
        }
    }
    // include service pack (if any)
    if (info.szCSDVersion)
    {
        ret += " ";
        ret += QString::fromWCharArray(info.szCSDVersion);
    }
    ret += QString(" (build %1)").arg(info.dwBuildNumber);
    if (info.dwMajorVersion >= 6)
    {
        if (sys.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
            ret += ", 64-bit";
        else if (sys.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
            ret += ", 32-bit";
    }

    return ret;    
}


#elif defined(LINUX_OS)

QString get_platform_name()
{
   // works for Ubuntu and possibly for Debian
   // most likely broken for other distros
   QFile file("/etc/lsb-release");
   if (!file.open(QIODevice::ReadOnly))
       return "GNU/Linux";

   QTextStream stream(&file);
   QString line;
   do
   {
       line = stream.readLine();
       if (line.isEmpty() || line.isNull())
           continue;
       
       QStringList split = line.split("=");
       if (split.size() != 2)
           continue;
       
       if (split[0] == "DISTRIB_DESCRIPTION")
       {
           // the value is double quoted, e.g. "Ubuntu 9.04", ditch the quotes
           QString val = split[1];
           val.chop(1);
           val.remove(0, 1);
           return val;
       }
   }
   while(!line.isNull());
   return "GNU/Linux";
}


#endif

} // app