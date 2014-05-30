Newsflash Plus
=========================

The world's best binary news reader!


Build Configuration
-------------------------

Build configuration is defined as much as possible in the config file in this folder.
It's assumed that either clang or gcc is used for a linux based build and msvc for windows.
A C++11 compliant compiler is required. 

Description of Modules
-------------------------

app/ 
    - newsflash application

app/lib
    - somewhat generic application helper code for example 
      logging, settings etc.

app/gui
    - newsflash gui code

corelib
   - low level engine implementation classes for sockets, file systems, usenet data baseses etc.

engine
   - generic downloader engine. provides a high level api to download data from the usenet.

keygen
    - newsflash keygen code

launcher 
    - a tool to help launch external processes and marshall kill indication back to the initial caller

media
    - media files 

msglib
    - simple message and message queue library

nzblib
    - NZB file format library

par2cmdline
    - 3rd party par2 parity repair command line tool

pylib
    - Qt python interface to execute python scripts

qjson
    - 3rd party Qt json library

rsslib
    - simple RSS library


sdk
    - newsflash extension/plugin interfaces


stringlib
    - boyer-moore fast string search implementation

testlib
    - unit test utility

unrar
    - 3rd party .rar archive extractor application

zlib
    - zlib compression library

Building for Linux
-------------------------

Download and extract boost package boost_1_51_0, then build and install boost.build

        $ tar -zxvvf boost_1_51_0.tar.gz
        $ cd boost_1_51_0/tools/build/v2/
        $ ./bootstrap.sh
        $ sudo ./b2 install
        $ bjam --version
        Boost.Build 2011.12-svn

Install the required packages for building Qt

        libx11-dev
        libext-dev
        libfontconfig-dev
        libxrender-dev
        libpng12-dev
        openssl-dev
        libgtk2.0-dev
        libgtk-3-dev
        libicu-dev
        autoconf
        qt4-qmake

Download and extract Qt everywhere and build it. Note that you must have XRender and fontconfig
for nice looking font rendering in Qt.

        $ tar -zxvvf qt-everywhere-opensource-src-4.8.2.tar.gz
        $ cd qt-everywhere-opensource-src-4.8.2
        $ ./configure --prefix=../qt-4.8.2 --no-qt3support --no-webkit
        $ make
        $ make install


Make a release package. 

        $ bjam release
        $ ./python build_package.py x.y.z


Building for Windows
----------------------------
Download and extract boost package boost_1_51_0, then build and install boost.build

        $ cd boost_1_51_0/tools/build/v2/
        $ bootstrap.bat
        $ b2
        $ set BOOST_BUILD_PATH=c:\boost-build\share\boost-build
        $ set PATH=%PATH%;c:\boost-build-engine\bin
        $ bjam --version
        Boost.Build 2011.12-svn

Build openssl. Install NASM and ActivePerl. 

        http://sourceforge.net/projects/nasm/        
        http://www.activestate.com/activeperl

        $ set PATH=%PATH%;"c:\Program Files (x86)\NASM\"
        $ cd openssl-1.0.1f
        $ perl configure VC-WIN32 --prefix=c:\coding\openssl_1_0_1f
        $ ms\do_nasm
        $ notepad ms\ntdll.mak
           * replace LFLAGS /debug with /release
        $ nmake -f ms\ntdll.mak
        $ nmake -f ms\ntdll.mak install

Par2cmdline
========================
Par2cmdline is a tool to repair and verify par2 files. The current version is 0.4.
http://sourceforge.net/projects/parchive/files/par2cmdline/

        $ ./configure
        $ make

Unrar
=========================
Unrar is a tool to unrar .rar archives. 
http://www.rarlab.com/rar_add.htm

        $ make
         
qjson
=======================
Qt JSON library 0.8.1
http://qjson.sourceforge.net/
https://github.com/flavio/qjson


    $ cmake -G "Unix Makefiles"
    $ make


Zlib
========================
The zlib compression library.

This zlib is here (in the project hiearchy) because it needs to be kept in sync
with the zlib code in python zlib module. Otherwise bad things can happen if the we have
two versions of zlib.so loaded in the same process with same symbol names.

So if zlib is updated at one place it needs to be updated at the other place as well!!

The current version is 1.2.5

