Welcome to Newsflash Plus SDK Readme
=====================================

Q: What is this about?
A: This SKD is the supporting softare for implementing Newsflash Plus plugins and extensions.


Q: What's a plugin?
A: A plugin is an object that "plugs" into the host application and provides some new functionality.
   Plugins are invisible to the user directly and are managed by the application. An example is a plugin
   that simply fetches a RSS feed from the internet.


Q: Whats an extension?
A: An extension is somewhat like a plugin except that whereas a plugin provides only simple categorized
   functionality an extension is free to do anything. The extension has access to more data within
   the application and is able to do it's own command processing. 


Q: Whats a command?
A: A command is a way for the interaction layer (typically the User Interface that the user is 
   interacting with) to communicate commands to the application. This design allows the 
   interaction layer to be independent of the application itself.

Technical Comments
=====================================

The commands are a separate implementation of the Qt QEvent system simply because
there's a need to communicate data back to the caller through the same command. 
For example the GUI might ask the application to fetch some data by sending a command
and when the send(...) returns the command contains the data requested. QEvent system
cannot do this easily. 