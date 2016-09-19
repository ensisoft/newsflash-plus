Newsflash Plus
=========================

The world's best binary news reader!

![Screenshot](https://raw.githubusercontent.com/ensisoft/newsflash-plus/master/screens/newsflash-downloads-win32.png "downloading from Usenet")

Build Configuration
-------------------------

Build configuration is defined as much as possible in the config file in this folder.
It's assumed that either clang or gcc is used for a linux based build and msvc for windows.
A C++11 compliant compiler is required. 

Only 32bit building is currently supported.


Description of Modules
-------------------------

app/
- newsflash application

app/gui/
- newsflash gui code

engine/
- generic downloader engine. provides a high level api to download data from the usenet.

keygen/
- newsflash keygen code


tools/par2cmdline
-------------------------
Par2cmdline is a tool to repair and verify par2 files. The original version was 0.4.
http://sourceforge.net/projects/parchive/files/par2cmdline/

*update* The original version has stalled to version 0.4. 
ArchLinux packages par2 from https://github.com/BlackIkeEagle/par2cmdline which is a fork off the original par2. 
The Newsflash par2 has been updated to version 0.6.11
("bump 0.6.11", https://github.com/BlackIkeEagle/par2cmdline/commit/7735bb3f67f4f46b0fcb88894f9a28fb2fe451c6)


tools/unrar
-------------------------
Unrar is a tool to unrar .rar archives. Current version 5.21 beta2
http://www.rarlab.com/rar_add.htm


third_party/qt-4.8.0
-------------------------
Qt Window/Application Framework/toolkit
https://download.qt.io/archive/qt/4.8/4.8.6/qt-everywhere-opensource-src-4.8.6.zip

Needs to be downloaded and extracted into third_party/

third_party/boost_1_51_0
--------------------------
High quality C++ libraries for stuff such as filesystem, parser generators etc.
https://www.boost.org

third_party/qjson
-------------------------
Qt JSON library 0.8.1
http://qjson.sourceforge.net/
https://github.com/flavio/qjson


third_party/zlib
-------------------------
The zlib compression library. The current version is 1.2.5
http://www.zlib.net/


third_party/protobuf
-------------------------
Google protobuffer library 2.6.1
https://github.com/google/protobuf

Use the protoc to compile the .proto files in the project.


third_party/openssl
------------------------
Secure Socket Layer & Cryptography. Current version is 1.0.1g
https://www.openssl.org/source/


Building for Linux
=======================

Start by cloning the source

```
   $ git clone https://github.com/ensisoft/newsflash-plus
   $ cd newsflash-plus
   $ mkdir dist_d
   $ mkdir dist
```


Download and extract boost package boost_1_51_0, then build and install boost.build

```
    $ cd third_party/boost_1_51_0/tools/build/v2/
    $ ./bootstrap.sh
    $ sudo ./b2 install
    $ bjam --version
    Boost.Build 2011.12-svn
```

Build openssl.

```
    nothing to be done for now since we're using the system openssl
```


Build Qt 

Note that you must have XRender and fontconfig for nice looking font rendering in Qt.

Install these packages.

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

```
    $ cd third_party/qt-4.8.6
    $ ./configure --prefix=~coding/qt-4.8.6 --no-qt3support --no-webkit
    $ make
    $ make install
```

NOTE: if you get this Cryptic error:
"bash: ./configure: /bin/sh^M: bad interpreter: No such file or directory" 
it means that the script interpreter is shitting itself on windows style line endings, so you probably downloaded
the .zip file instead of the .tar.gz  (you can fix this with dos2unix, but then also the executable bits are not set
and configure will fail with some other cryptic error such as "no make or gmake was found bla bla".

Build zlib

```
    $ cd third_party/zlib
    $ bjam release
```    

Build protobuf

```
    $ cd third_party/protobuf
    $ ./configure
    $ make
    $ mkdir lib
    $ cp src/.libs/libprotobuf.a lib/libprotobuf.a
```    

WARNING: This might shit itself if aclocal has been updated to another version.

```
    WARNING: 'aclocal-1.14' is missing on your system.
         You should only need it if you modified 'acinclude.m4' or
         'configure.ac' or m4 files included by 'configure.ac'.
         The 'aclocal' program is part of the GNU Automake package:
         <http://www.gnu.org/software/automake>
         It also requires GNU Autoconf, GNU m4 and Perl in order to run:
         <http://www.gnu.org/software/autoconf>
         <http://www.gnu.org/software/m4/>
         <http://www.perl.org/>
    Makefile:641: recipe for target 'aclocal.m4' failed
 ```

You can try to fix it by opening and editing protobuf/Makefile 

```
L240: ACLOCAL = ${SHELL} /home/enska/coding/newsflash/protobuf/missing <del>aclocal-1.14</del>
L240: ACLOCAL = ${SHELL} /home/enska/coding/newsflash/protobuf/missing aclocal-1.15

L246: AUTOMAKE = ${SHELL} /home/enska/coding/newsflash/protobuf/missing <del>automake-1.14</del>
L246: AUTOMAKE = ${SHELL} /home/enska/coding/newsflash/protobuf/missing automake-1.15

```



Build qjson

NOTE: I have edited the CMakeList.txt to have a custom Qt path. 

```
    $ cd third_party/qjson
    $ cmake -G "Unix Makefiles"
    $ make
```    

Build zlib

```
    $ cd third_party/zlib
    $ bjam release
    
```

Build par2cmdline

```
     $ cd tools/par2cmdline
     $ aclocal
     $ automake --add-missing
     $ autoconf
     $ ./configure
     $ sed -i 's/-g -O2/-O2/g' Makefile
     $ make
     $ cp par2 ~/coding/newsflash/dist
     $ cp par2 ~/coding/newsflash/dist_d
```     

Build  unrar

```
    $ cd tools/unrar
    $ make
    $ cp unrar ~/coding/newsflash/dist
    $ cp unrar ~/coding/newsflash/dist_d
```



Comments about ICU.
Both Qt and boost.regex depend on ICU. So if ICU updates both Qt and boost.regex needs to be rebuilt.



Building for Windows
=======================

NOTE About WindowsXP. To target WinXP we need /SUBSYSTEM:WINDOWS,5.01
More information here:

http://www.tripleboot.org/?p=423
http://blogs.msdn.com/b/vcblog/archive/2012/10/08/windows-xp-targeting-with-c-in-visual-studio-2012.aspx?Redirected=true

Currently only 32bit build is supported. You'll need Microsoft Visual Studio 2013. 
Once you've installed visual studio open the VS2013 x86 Native Tools Commmand Prompt.

Start by cloning the source

```
    $ git clone https://github.com/ensisoft/newslash-plus
    $ cd newsflash-plus
    $ mkdir dist_d
    $ mkdir dist
```

Download and extract boost package boost_1_51_0, then build and install boost.build

```
    $ cd third_party/boost_1_51_0/tools/build/v2/
    $ bootstrap.bat
    $ b2
    $ set BOOST_BUILD_PATH=c:\boost-build\share\boost-build
    $ set PATH=%PATH%;c:\boost-build-engine\bin
    $ bjam --version
    Boost.Build 2011.12-svn
```    

Build openssl. Install NASM and ActivePerl. 

    http://sourceforge.net/projects/nasm/        
    http://www.activestate.com/activeperl

Note that openssl does not maintain source code compatibility between versions. And this applies
even to minor versions. I.e. openssl 1.0.2 is source compatible with openssl 1.0.1
Qt seems to want the 1.0.1x series.

```
    $ set PATH=%PATH%;"c:\Program Files (x86)\NASM\"
    $ set PATH=%PATH%;"c:\Perl64\bin"
    $ cd third_party/openssl
    $ perl configure VC-WIN32 --prefix="%cd%\sdk"
    $ ms\do_nasm
    $ notepad ms\ntdll.mak
      * replace LFLAGS /debug with /release
    $ nmake -f ms\ntdll.mak
    $ nmake -f ms\ntdll.mak install
```

Build Qt

```
    $ cd third_party/qt-4.8.6
    $ configure.exe -no-qt3support -no-webkit -debug-and-release -openssl -I "%cd%\..\openssl\sdk\include" -L "%cd%\..\openssl\sdk\lib"
    $ nmake
    $ nmake install
```    

Build zlib

```
    $ cd third_party/zlib
    $ bjam release
```    


Build protobuf library

```
    $ cd third_party/protobuf
    $ cd vsprojects
    $ msbuild protobuf.sln /p:Configuration=Release
    $ msbuild protobuf.sln /P/Configuration=Debug
```

Build qjson

NOTE: I have edited the CMakeList.txt to have a custom Qt path. 

``` 
    $ cd third_party/qjson
    $ cmake -G "Visual Studio 12 2013" 
    $ msbuild  qjson.sln /p:Configuration=Release
```

Build zlib

```
    $ cd third_party/zlib
    $ bjam release
    
```


Build Par2cmdline

```
    $ cd tools/par2cmdline
    $ msbuild par2cmdline.sln /p:Configuration=Release
``` 

Build Unrar

```
    $ cd tools/unrar
    $ msbuild UnRAR.vcxproj /p:Configuration=Release
```


UX Guidelines
========================

Active user context
------------------------

User actions happen either immediately in the current user context when input from user 
is processed (synchronous) or alternatively they complete later in time (async).

When an synchronous operation is performed we can ask user for confirmation (if we must)
through modal dialogs. Examples are choosing the file to open, choosing folder, asking for 
account to be used etc. Also if the operation is cannot be performed (the file cannot be opened etc)
It's preferrable to indicate the failure immediately to the user using an Error dialog.

However if the operation is enqueued to background operation, any events generated later on
such as information, damage/completion or error reports must be go to the application log.
We may not interrupt the user. 


Visual indications
-------------------------

When there's new data  such as a new event, new download etc. the relevant tab
should visually indicate this. 

Limit options
-------------------------

todo


Limit modal dialogs
-------------------------

todo




Design Babblings
========================


What is an error what is not an error? 
--------------------------------------
From a user perspective an error is something that
is unexpected but still potentially part of normal program behaviour. Examples are 
"file not found", or "file system is full/unwritable", "network connection went down",
kind of errors. Also errors that occur when some OS resource (memory, mutex, thread, event etc.)
allocation fails fall into this category. 
These howerver from the application/programmer perspective are *not* errors. 
In this text we refer to this first category of errors as "soft errors".

The second category of errors are errors that are not part of the normal application behaviour,
but are in fact errors made by the programmer. These include errors such as memory corruption,
out of bounds access and other undefined behaviours  or other bugs in application logic. 
The category of these kind of errors is wide and varied. 
In this text we refer to this category of errors as "hard errors" or "programmer errors."

How to deal with soft errors?
--------------------------------------
If the user invokes an operation that is performed immediately and the response is 
available immediately the preferred action is to inform the user immediately. 
This is most sensibly done through a message box that carries the error details.

For example user wants to open an file that is not readable. Preferable we can provide 
immediate feedback to the user to notify that the file could not be opened.

However the same stragegy does not work for errors that occur outside the current
user "context". I.e. problems that araise during excution that happens on the background.
The only sensible thing is to log these events in the application log. And to
indicate error status in the task/action/item object in the UI as well. 

For these we distinguish a simple 3 level system.

    * Information
    * Warning
    * Error

Information is simply not an error at all. It's simply just information about some
event that occurred in the past.

Warning is used in cases where some action may not complete correctly or completely 
but such action might allow processing to continue nevertheless. Examples are for 
for example downloaded file being damaged (maybe we can repair it) or some non-important
data file not being found (like some history file of downloaded files). Possibly 
inconvenient and not perfect but life goes on.

Error is used in cases where we encounter a soft error. These nearly always originate from 
some system call and should be associated with system error message and resources 
in question. For example TCP connection could not be created because the host rejected, 
or file could not be opened because we didnt have permission. Or some resource allocation
failed.  These cases might mean that processing that encountered the error cannot continue.


How to deal with hard errors?
------------------------------------
Ok then the nasty stuff, the bugs that the programmers write. We have 3 potential
mechanisms how to deal with these conditions. 

    * throw an exception ?
    * assert ?
    * ASSERT ?


Throwing an exception (when applicable) is in fact not a recommended way to deal with these errors. 
Simply because this will mask the real problem. Throwing an exception will also make debugging
harder since the exception carries no information where it occurred and we have no stack trace.

The standard assert() macro is a nifty and simple utility. However it's value is limited
since it's usually only built in debug builds. However we can use the standard assert to trap
errors that are likely to be quickly encountered during development. Example cases are 
using asserts to verify pre and post conditions inside a class. (Is the file handly really open,
did that expression really evalute to 'true', was that pointer definitely non-null. 

However when using assert we must be sure that there's no way for an error condition to fall through
the cracks and possibly lead to incorrect behaviour at runtime. 

ASSERT is a the big cousin of the standard assert. Except that it will always be built in. And it will
always terminate the process with a big bang and a core dump when it is violated. Sounds harsh? 
Trust me *this* is a *good* thing. Why? Because it allows us to pinpoint the exact location where 
the problem occurred. It gives us a stacktrace and a memory dump for post mortem debugging thereby 
allowing us to fix the real problem. 

The recommended guideline is to user ASSERT always when 

a) we're not 100% that we have covered all the loopholes in development with normal assert. 
b) violation of the condition would lead to undefined behaviour. (out of bounds access, null pointer, etc.)


Finally a note about resource cleanup. Resource cleanup should always use ASSERT (to check the return value). Why? 
Because it's likely to happen in a destructor (so we cant throw anyway. this would give us a non-controlled abortion).
And if the resource cleanup fails the only viable condition to trigger this is
corrupted application state. Examples are double free, or double delete, trying to close a handle which
has become corrupted (incorrect handle value), double closing etc.  Again, better just to dump core
asap, investiage and then fix the application.



Signals and Slots or std::function<> ?
=======================================

Signals & Slots is the Qt mechanism for connecting events from senders to receivers. 
It is not based on standard c++ facilities and requires tool support (MOC) in order to work.
    
Pros of Signals and Slots.
    + Support in the Qt designer 
    + Built in threading support for queued signals when necessary.
    + Built in support for multiple receivers

Cons of Signals and Slots.
    - Requires separate tool to work (MOC)
    - Requires that types supporting (either emitting or receiving) derive from QObject 
    - Runtime connection mechanism which results in runtime errors rather than compile time errors. (error prone)
    - Requires Q_DECLARE_METATYPE for custom types


std::function is the standard (c++11) facility for defining generic function objects.

Pros of std::function
    + Standard facility. Only requirement is c++11 enabled compiler.
    + Type safe
    + Compile time errors
    + Generic implementation can bind to lambdas and std::bind function objects

Cons of std::function
    - Only single receiver


Current design guideline is to use Qt's signals slots in obvious cases that are required by the toolkit,
such as when handling widget events or other Qt types events. (such QProcess, QThread, etc).
Also when there's a need to send/receive "broadcast" type signals such as file completion from the engine
or new system event. These are best handled leveraging the Qt's signals and slots.

However when we have a case where some lower level component who's scope is very limited wants to communicate
information upwards only to a single listener this is easily accomplished through the use of std::function.
Example is ParityChecker objects communicating state changes upwards (to Repairer object). This communication
is only limited to inside Repairer and happens between ParityChecker and Repairer only. 