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
#  include <QtGui/QDesktopServices>
#  include <QFile>
#  include <QTextStream>
#  include <QIODevice>
#  include <QStringList>
#  include <QDir>

#include <newsflash/warnpop.h>

#if defined(WINDOWS_OS)
#  include <windows.h>
#endif
#if defined(LINUX_OS)
#  include <sys/vfs.h>
#endif

#include "platform.h"
#include "format.h"

namespace app
{

#if defined(WINDOWS_OS)

QIcon extractIcon(const QString& binary)
{
    HICON small_icon[1] = {0};
    ExtractIconEx(binary.utf16(), 0, NULL, small_icon, 1);
    if (small_icon[0])
    {
        bool foundAlpha  = false;
        HDC screenDevice = GetDC(0);
        HDC hdc = CreateCompatibleDC(screenDevice);
        ReleaseDC(0, screenDevice);

        ICONINFO iconinfo = {0};
        bool result = GetIconInfo(small_icon[0], &iconinfo); //x and y Hotspot describes the icon center
        if (result)
        {
            int w = iconinfo.xHotspot * 2;
            int h = iconinfo.yHotspot * 2;

            BITMAPINFOHEADER bitmapInfo;
            bitmapInfo.biSize        = sizeof(BITMAPINFOHEADER);
            bitmapInfo.biWidth       = w;
            bitmapInfo.biHeight      = h;
            bitmapInfo.biPlanes      = 1;
            bitmapInfo.biBitCount    = 32;
            bitmapInfo.biCompression = BI_RGB;
            bitmapInfo.biSizeImage   = 0;
            bitmapInfo.biXPelsPerMeter = 0;
            bitmapInfo.biYPelsPerMeter = 0;
            bitmapInfo.biClrUsed       = 0;
            bitmapInfo.biClrImportant  = 0;
            DWORD* bits;
            
            HBITMAP winBitmap = CreateDIBSection(hdc, (BITMAPINFO*)&bitmapInfo, DIB_RGB_COLORS, (VOID**)&bits, NULL, 0);
            HGDIOBJ oldhdc = (HBITMAP)SelectObject(hdc, winBitmap);
            DrawIconEx( hdc, 0, 0, small_icon[0], iconinfo.xHotspot * 2, iconinfo.yHotspot * 2, 0, 0, DI_NORMAL);
            QImage image = qt_fromWinHBITMAP(hdc, winBitmap, w, h);
            
            for (int y = 0 ; y < h && !foundAlpha ; y++) 
            {
                QRgb *scanLine= reinterpret_cast<QRgb *>(image.scanLine(y));
                for (int x = 0; x < w ; x++) 
                {
                    if (qAlpha(scanLine[x]) != 0) 
                    {
                        foundAlpha = true;
                        break;
                    }
                }
            }
            if (!foundAlpha) 
            {
                //If no alpha was found, we use the mask to set alpha values
                DrawIconEx( hdc, 0, 0, small_icon[0], w, h, 0, 0, DI_MASK);
                QImage mask = qt_fromWinHBITMAP(hdc, winBitmap, w, h);
                
                for (int y = 0 ; y < h ; y++)
                {
                    QRgb *scanlineImage = reinterpret_cast<QRgb *>(image.scanLine(y));
                    QRgb *scanlineMask = mask.isNull() ? 0 : reinterpret_cast<QRgb *>(mask.scanLine(y));
                    for (int x = 0; x < w ; x++)
                    {
                        if (scanlineMask && qRed(scanlineMask[x]) != 0)
                            scanlineImage[x] = 0; //mask out this pixel
                        else
                            scanlineImage[x] |= 0xff000000; // set the alpha channel to 255
                    }
                }
            }
            //dispose resources created by iconinfo call
            DeleteObject(iconinfo.hbmMask);
            DeleteObject(iconinfo.hbmColor);
        
            SelectObject(hdc, oldhdc); //restore state
            DeleteObject(winBitmap);
            DeleteDC(hdc);        
            return QIcon(QPixmap::fromImage(image));
        }
    }
}

QString getPlatformName()
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

QString resolveMountPoint(const QString& directory)
{
    QDir dir(directory);
    QString path = dir.canonicalPath();
    if (path.isEmpty())
        return "";

    // extract the drive letter
    QString ret;
    ret.append(path[0]);
    ret.append(":");
    return ret;
}

quint64 getFreeDiskSpace(const QString& filename)
{
    const auto& native = QDir::toNativeSeparators(filename);

    ULARGE_INTEGER large = {};
    if (!GetDiskFreeSpaceEx(native.utf16(), &large, nullptr, nullptr))
        return 0;

    return large.QuadPart;
}

void openFile(const QString& file)
{
    HINSTANCE ret = ShellExecute(nullptr,
        L"open",
        file.utf16(),
        NULL, // executable parameters, don't care
        NULL, // working directory
        SW_SHOWNORMAL);
}

void openWeb(const QString& url)
{
    // todo:
}

void shutdownComputer()
{
    //todo:
}

#elif defined(LINUX_OS)

QIcon extractIcon(const QString& binary)
{
    return QIcon();
}

QString getPlatformName()
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

QString resolveMountPoint(const QString& directory)
{
    QDir dir(directory);
    QString path = dir.canonicalPath(); // resolves symbolic links

    // have to read /proc/mounts and compare the mount points to 
    // the given directory path. an alternative could be /etc/mtab
    // but apparently /proc/mounts is more up to date and should exist
    // on any new modern kernel
    QFile mtab("/proc/mounts");
    if (!mtab.open(QIODevice::ReadOnly))
        return "";
    
    QStringList mounts;
    QString line;
    QTextStream stream(&mtab);
    do 
    {
        line = stream.readLine();
        if (line.isEmpty() || line.isNull())
            continue;
        
        QStringList split = line.split(" ");
        if (split.size() != 6)
            continue;
        
        // /proc/mounts should look something like this...
        // rootfs / rootfs rw 0 0
        // none /sys sysfs rw,nosuid,nodev,noexec 0 0
        // none /proc proc rw,nosuid,nodev,noexec 0 0
        // udev /dev tmpfs rw 0 0
        // ...
        mounts.append(split[1]);
    }
    while (!line.isNull());

    QString mount_point;
    for (int i=0; i<mounts.size(); ++i)
    {
        if (path.startsWith(mounts[i]))
        {
            if (mounts[i].size() > mount_point.size())
                mount_point = mounts[i];
        }
    }
    return mount_point;    
}

quint64 getFreeDiskSpace(const QString& filename)
{
    const auto native = QDir::toNativeSeparators(filename);

    const auto u8 = narrow(native);

    struct statfs64 st = {};
    if (statfs64(u8.c_str(), &st) == -1)
        return 0;

    // f_bsize is the "optimal transfer block size"
    // not sure if this reliable and always the same as the
    // actual file system block size. If it is different then
    // this is incorrect.  
    auto ret = st.f_bsize * st.f_bavail;    
    return ret;
}

// gnome alternatives
// gnome-open
// gnome-session-quit --power-off --no-prompt

QString openfile_command = "xdg-open";
QString shutdown_command = "systemctl poweroff";

void setOpenfileCommand(const QString& cmd)
{
    openfile_command = cmd;
}

void setShutdownCommand(const QString& cmd)
{
    shutdown_command = cmd;
}

void openFile(const QString& file)
{
    QDesktopServices::openUrl("file:///" + file);
}

void openWeb(const QString& url)
{
    QDesktopServices::openUrl(url);
}

void shutdownComputer()
{
    // todo:
}

QString getOpenfileCommand()
{
    return openfile_command;
}

QString getShutdownCommand()
{
    return shutdown_command;
}


#endif

} // app