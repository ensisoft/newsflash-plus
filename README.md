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


qjson
-------------------------
Qt JSON library 0.8.1
http://qjson.sourceforge.net/
https://github.com/flavio/qjson


zlib
-------------------------
The zlib compression library. The current version is 1.2.5
This zlib is here (in the project hiearchy) because it needs to be kept in sync
with the zlib code in python zlib module. Otherwise bad things can happen if the we have
two versions of zlib.so loaded in the same process with same symbol names.

So if zlib is updated at one place it needs to be updated at the other place as well!!


protobuf
-------------------------
Google protobuffer library 2.6.1

Use the protoc to compile the .proto files in the project.




Building for Linux
=======================

Start by cloning the source

   $ cd ~
   $ mkdir coding
   $ cd coding
   $ git clone https://bitbucket.org/ensisoft/newsflash.git 
   $ cd newsflash
   $ mkdir dist_d
   $ mkdir dist


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

    $ tar -zxvvf qt-everywhere-opensource-src-4.8.6.tar.gz
    $ cd qt-everywhere-opensource-src-4.8.6
    $ ./configure --prefix=~coding/qt-4.8.6 --no-qt3support --no-webkit
    $ make
    $ make install

NOTE: if you get this Cryptic error:
"bash: ./configure: /bin/sh^M: bad interpreter: No such file or directory" 
it means that the script interpreter is shitting itself on windows style line endings, so you probably downloaded
the .zip file instead of the .tar.gz  (you can fix this with dos2unix, but then also the executable bits are not set
and configure will fail with some other cryptic error such as "no make or gmake was found bla bla".

Build zlib

    $ cd zlib
    $ bjam release

Build protobuf

    $ cd protobuf
    $ ./configure
    $ make
    $ mkdir lib
    $ cp src/.libs/libprotobuf.a lib/libprotobuf_d.a
    $ cp src/.libs/libprotobuf.a lib/libprotobuf_r.a
    $ cp src/protoc ~/coding/newsflash/engine/
    $ cd ~/coding/newsflash/engine/
    $ ./protoc session.proto --cpp_out=.

Build qjson

NOTE: I have edited the CMakeList.txt to have a custom Qt path. 

    $ cd qjson
    $ cmake -G "Unix Makefiles"
    $ make

Build par2cmdline

     $ cd tools/par2cmdline
     $ ./configure
     $ make
     $ cp par2 ~/coding/newsflash/dist
     $ cp par2 ~/coding/newsflash/dist_d

Build  unrar

    $ cd tools/unrar
    $ make
    $ cp unrar ~/coding/newsflash/dist
    $ cp unrar ~/coding/newsflash/dist_d


Building for Windows
=======================

NOTE About WindowsXP. To target WinXP we need /SUBSYSTEM:WINDOWS,5.01
More information here:

http://www.tripleboot.org/?p=423
http://blogs.msdn.com/b/vcblog/archive/2012/10/08/windows-xp-targeting-with-c-in-visual-studio-2012.aspx?Redirected=true

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


Download and extract Qt everywhere to a location where you want it installed for example c:\coding\qt-4.8.6

    $ configure.exe -no-qt3support -no-webkit -debug-and-release -openssl -I c:\coding\openssl_1_0_1f\include -L c:\coding\openssl_1_0_1f\lib


protobuf

    - open the solution in vsprojects and build the library
    - copy vsprojects/Debug/libprotobuf.lib to ../lib/libprotobuf_d.lib
    - copy vsprojects/Release/libprotobuf.lib to ../lib/libprotobuf_r.lib


Par2cmdline


Unrar



Potential (New) Features
=========================

- disk size checker, notify / react to low disk conditions.
- joining of hjsplit files
- some sort of searching in the headers
- auto downloader/scheduler for TV series for example
- support for nzbindex.nl, binsearch.info or other similar registration-free backend
- help files
- installer/uninstaller


Bugs
========================


- assertion in the engine when quitting with active update ()
- batch eta is messed up
- update eta is messed up
- fill server is used for non-fillable downloads (from headers)
- killing the task doesn't update headers ui 
- archive repair doesnt alwys work
- sometimes ssl connections stop working
- headers update will download faster than it can consume, needs throttling
- unrar progress bar doesnt always work
+ search modifiers link doesn't do anything
+ search button changes incorrectly
+ command pipelining with ssl socket is broken


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