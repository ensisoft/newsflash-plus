Welcome to Newsflash Plus Readme file


Newsflash Plus is a Usenet binary reader. If you're reading this
readme I presume you're already well on your way to using the 
software. I hope you enjoy it.

If you find any bugs or problems or just feel like you have
a great idea for a new feature dont't hesitate to contact me.

Visit these online resources to find out more.

http://www.ensisoft.com/
http://www.ensisoft.com/forum

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

   2. use gnome-session-quit (or equivalent. Consult your Distribution docs)
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
