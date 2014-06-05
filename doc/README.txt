Welcome to Newsflash Plus Readme file


Newsflash Plus is a Usenet binary reader. If you're reading this
readme I presume you're already well on your way to using the 
software. I hope you enjoy it.

If you find any bugs or problems or just feel like you have
a great idea for a new feature dont't hesitate to contact me.

I recommend you check the documentation for tips on how to
make the most ouf of the software.

Known Bugs:

- Filter settings are not correctly applied if group is reloaded.

Known problems on Microsoft Windows XP:

- If you install the application to path that has Unicode characters
  (for example Japanese, Chinese etc), Python will not work.
  This is a limitation of Python not Newsflash Plus.

- If you download to a path that has Unicode characters 
  (for example Japanese, Chinese etc), automatic par2 repair 
  and .rar extraction will not work. This is not a limitation of Newsflash Plus
  but a problem in the tools.  

- There is no uninstaller. If you want to uninstall the application
  simply find the installation folder (e.g. C:\Program Files\Newsflash on standard installation)
  and delete the whole folder.
  Configuration files can be removed by deleting the config folder C:\Documents And Settings\USERNAME\.newsflash
  There are no registry entries created by the program

- Sometimes after opening a file open dialog the window on the background goes blank.
  As a work around try using a non-default theme, such as Windows XP.

- Sometimes opening folders (Go to folder) hangs. This seems to be a Windows Explorer issue.


Known problems on Linux Desktops:
- If you're running Gnome sometimes not all icons (in menus and buttons) are visible.
  The fix for this is to run the following commands:
  
  gconftool-2 --type boolean --set /desktop/gnome/interface/buttons_have_icons true
  gconftool-2 --type boolean --set /desktop/gnome/interface/menus_have_icons true

- GTK+ theme not working properly. (Will either complain about QGtkTheme not being able to detect them or then crash)


Shutting down PC on Linux machines:

There are several ways to attempt to shutdown the PC.
   1. "/sbin/shutdown" requires super user permissions. 

   To enable this, make a new group in /etc/group something like
    "shutdown:x:123:MYUSERNAME"
   Change /sbin/shutdown group to shutdown
    "chown root:shutdown /sbin/shutdown"
   Give execution permission for group
    "chmod 750 /sbin/shutdown"
   Set sudo bit on
    "chmod u+s /sbin/shutdown"

   Now any user in shutdown group can shutdown the PC using shutdown command. Note that 
   the new groups are not effective, untill log off/log on is done. newgrp can be used to change
   into new group immediately.

   2. use gnome-session-quit. 
   This command may not be available on the older Gnome/Ubuntu installations. Also if you run KDE you won't have it. 
   KDE probably has somethig similar.

   3. Use D-BUS with dbus-send
   dbus-send --system --print-reply --dest=org.freedesktop.ConsoleKit /org/freedesktop/ConsoleKit/Manager  org.freedesktop.ConsoleKit.Manager.Stop

   4. use systemctl
   systemctl poweroff

   Hopefully one of these works for you.


Shutting down PC on Windows machines:
  Newsflash needs to be run with adminstrator privileges


www.ensisoft.com
