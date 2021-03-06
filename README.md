Newsflash Plus
=========================

The world's best binary news reader!

![Screenshot](https://raw.githubusercontent.com/ensisoft/newsflash-plus/master/screens/newsflash-downloads-win32.png "downloading from Usenet")

Build Configuration
-------------------------

Build configuration is defined as much as possible in the config file in this folder.
It's assumed that either GCC is used for a linux based build and MSVC for windows.
A C++11 compliant compiler is required.

Only 64bit building is currently supported.

A new version of CMake is required (currently works with CMake >= 3.9.1).
If you get an error about automoc producing files by the same name your CMake version is too old.
See this link for more information:  
https://public.kitware.com/Bug/view.php?id=12873


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
Unrar is a tool to unrar .rar archives. Current version ~~5.21 beta2~~ 5.50 beta1  
http://www.rarlab.com/rar_add.htm

third_party/zlib
-------------------------
The zlib compression library. The current version is 1.2.11  
http://www.zlib.net/

third_party/openssl
------------------------
Secure Socket Layer & Cryptography. Current version is 1.0.2k  
https://www.openssl.org/source/

Building for Linux
=======================

You need to install the Qt5 and boost development libraries. 

Start by cloning the source

```
   $ git clone https://github.com/ensisoft/newsflash-plus
   $ cd newsflash-plus
   $ mkdir dist_d
   $ mkdir dist
```

Build openssl.

```
    $ cd third_party/openssl
    $ ./config --openssldir=`pwd`/sdk shared
    $ make
    $ make install
```

Build zlib

```
    $ cd third_party/zlib
    $ cmake -G "Unix Makefiles"
    $ make
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

Build Newsflash

```
  $ cd newsflash_plus
  $ mkdir build_d
  $ cd build_d
  $ cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug ..
  $ make
  $ make install
  $ cd ..
  $ mkdir build
  $ cd build
  $ cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ..
  $ make
  $ make install

```

Run testcases
```
  $ cd build_d
  $ cp -r ../test_data .
  $ ./test_server &
  $ ctest
  $ killall test_server
```

Comments about ICU.
Both Qt and boost.regex depend on ICU. So if ICU updates both Qt and boost.regex needs to be rebuilt.


Building for Windows
=======================

NOTE About WindowsXP. To target WinXP we need /SUBSYSTEM:WINDOWS,5.01
More information here:

http://www.tripleboot.org/?p=423  
http://blogs.msdn.com/b/vcblog/archive/2012/10/08/windows-xp-targeting-with-c-in-visual-studio-2012.aspx?Redirected=true

Currently only 64bit build is supported. You'll need Microsoft Visual Studio 2019.
Once you've installed visual studio open the VS2019  x64 Native Tools Command Prompt.

Install Qt 5.12.6 (At the time of writing there's only MSVS2017 pre-built binaries)  
https://download.qt.io/archive/qt/5.12/5.12.6/

About Qt and OpenSSL. The documentation at https://doc.qt.io/qt-5/ssl.html#import-and-export-restrictions
says that the OpenSSL libraries are / can be installed by the Qt installer. This looks like a lie, the
installer offers no option to install these. (Currently tried with qt-opensource-windows-x86-5.12.6.exe)

Qt 5x requires OpenSSL 1.1.x (See QSsslSocket::sslLibraryBuildVersionString).

The OpenSSL Wiki below provides links to sites hosting prebuilt OpenSSL binaries but they're all outdated
OpensSL 1.0.0x versions.  
https://wiki.openssl.org/index.php/Binaries

As of now there's this site:  https://kb.firedaemon.com/support/solutions/articles/4000121705
that has a 1.1.x prebuilt library package that seem to work with Qt5.  
https://mirror.firedaemon.com/OpenSSL/openssl-1.1.1e-dev.zip  

Install Boost.1.72.0 (currently only MSVS2017 pre-built binaries)
https://sourceforge.net/projects/boost/files/boost-binaries/1.72.0/boost_1_72_0-msvc-14.2-64.exe/download

Start by cloning the source
```
    $ git clone https://github.com/ensisoft/newslash-plus
    $ cd newsflash-plus
    $ mkdir dist_d
    $ mkdir dist
```

Build openssl. Install ActivePerl.

    http://www.activestate.com/activeperl

Note that openssl does not maintain source code compatibility between versions. And this applies
even to minor versions. I.e. openssl 1.0.2 is source compatible with openssl 1.0.1
Qt seems to want the 1.0.1x series.

Also note that the because of the apparent INCOMPETENCE of OpenSSL developers we're locked to building
the OpenSSL with MSVS2013. Several versions of OpenSSL starting from 1.0.1k to 1.0.2u fail to build
with either MSVS2017 or MSVS2019. Sample of build (compile/link) errors:

* .\crypto\bn\bn_exp.c(508): error C2065: 'BN_FLG_FIXED_TOP': undeclared identifier
* .\crypto\bn\bn_exp.c(685): error C2065: 'BN_FLG_FIXED_TOP': undeclared identifier
* .\crypto\bio\bss_file.c(434): error C2065: 'SYS_F_FFLUSH': undeclared identifier
 * fatal error LNK1112: module machine type 'x64' conflicts with target machine type 'x86'

It takes a special kind of code and programmers to make C code fail like this across compilers.

So therefore the OpenSSL libraries are produced with MSVS2013.

```
    $ set PATH=%PATH%;"c:\Perl64\bin"
    $ cd third_party/openssl
    $ perl configure VC-WIN64A  --prefix="%cd%\sdk"
    $ ms\do_win64a
    $ ~~notepad ms\ntdll.mak~~
    $ notepad ms\nt.mak
      * replace LFLAGS /debug with /release
    $ nmake -f ms\nt.mak
    $ nmake -f ms\nt.mak install
```

Build zlib

```
    $ cd third_party/zlib
    $ cmake -G "Visual Studio 16 2019"
    $ msbuild zlib.sln /p:Configuration=Release /p:Platform=x64
```

Build Par2cmdline

```
    $ cd tools/par2cmdline
    $ msbuild par2cmdline.sln /p:Configuration=Release /p:Platform=x64
```

Build Unrar

```
    $ cd tools/unrar
    $ msbuild UnRAR.vcxproj /p:Configuration=Release /p:Platform=x64
```

Build 7zip (7za) or NOT
```
    The source package is outdated without proper build files. Fuck this shit.
    Instead grab the pre-built binary from http://www.7-zip.org/download.html
    The extra package has a command line version and apparenty is built
    against static crt.
```

Build Newsflash

```
    $ cd newsflash_plus
    $ mkdir build
    $ cd build
    $ cmake -G "Visual Studio 16 2019" -DCMAKE_BUILD_TYPE=Release ..
    $ msbuild NewsflashPlus.sln /p:Configuration=Release /p:Platform=x64
    $ mkdir build_d
    $ cmake -G "Visual Studio 12 2019 Win64" -DCMAKE_BUILD_TYPE=Debug ..
    $ msbuild NewsflashPlus.sln /p:Configuration=Debug /p:Platform:x64

```

Run Unit Tests
```
    $ cd newsflash_plus\build_d
    $ copy ..\test_data .
    $ start Debug\test_server.exe
    $ ctest -C Debug
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