Newsflash Plus
=========================

The world's best binary news reader!


BUILD CONFIGURATION

Build configuration is defined as much as possible in the config file in this folder.
It's assumed that either clang or gcc is used for a linux based build and msvc for windows.
A C++11 compliant compiler is required. 

BUILDING
todo:
- setup boost.build
- build Qt
- build 3rd party binaries
- openssl

RELEASING

For Windows

NullSoft Installer:
I've made changes to the Base.nsh file. This script template now contains a script code
to make a shortcut in the start menu. Updated Base.nsh should be copied into 

               "c:\program files\NSIS\contrib\zip2exe\base.nsh" 

before using the zip2exe tools.

1. Make a release build, make sure contents in dist/ are correct
2. Compare dist and dist_final, update all changed files
3. Make a .zip file by selecting the contents of dist_final and write .zip to releases/ e.g. newsflash_plus_3.1.0_rc1.zip
4. Run NSIS Zip2Exe tool and create a self extracting .exe file
5. Test the exe


For Linux

1. Make a release build
2. Compare dist and dist_final, update files (binaries, help file, python, extensions and other .so files)
    NOTE THAT MELD WILL NOT BY DEFAULT NOT COMPARE .so FILES!!
3. $ tar -cvvf newsflash_plus_X.X.X.tar dist_final/
4. gzip newsflash_plus_X.X.X.tar
5. mv newsflash_plus_X.X.X.tar.gz releases/
6. Extract and test




Par2cmdline
========================
Par2cmdline is a tool to repair and verify par2 files. 
The current version is 0.4.

You can download the source from:
http://sourceforge.net/projects/parchive/files/par2cmdline/

To build:

$ ./configure
$ make



Unrar
=========================
Unrar is a tool to unrar .rar archives. 

You can download it from:
http://www.rarlab.com/rar_add.htm

To build:

$ make


Zlib
========================
The zlib compression library.

This zlib is here (in the project hiearchy) because it needs to be kept in sync
with the zlib code in python zlib module. Otherwise bad things can happen if the we have
two versions of zlib.so loaded in the same process with same symbol names.

So if zlib is updated at one place it needs to be updated at the other place as well!!

The current version is 1.2.5