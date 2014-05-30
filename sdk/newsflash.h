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
  
#pragma once

#include <newsflash/warnpush.h>
  #include <QtGui/QIcon>
  #include <QString>
  #include <QList>
#include <newsflash/warnpop.h>
#include "filetype.h"
#include "types.h"

class QVariant;
class QStringList;

namespace sdk
{
    // Log event type
    enum class log 
    {
        info,         // Information
        warning,      // Warning
        error,        // Serious error
        debug         // debugging info, only enabled in debug builds
    };


    // Settings enum lists the settings available to the 
    // sdkension through the host application
    enum class settings
    {
        istallation_dir,  // QString, application installation directory
        home_dir,         // QString, user's home directory path
        config_dir,       // QString, config dir
        data_dir,         // QString, currently configured data directory
        log_dir,          // QString, currently configured log directory
        download_dir,     // Qstring, currently configured donwload dir
        help_dir,         // QString, application help directory (where the help files are installed)
        preferred_service // QString, currently preferred service if any
    };

    // Tool represents an external application (tool) 
    // that has been configured in the host application. 
    // Didfferent tools have different purposes for example
    // repairing archives (par2) or extracting archives (winrar)
    struct tool 
    {
        QString name;           // user configured name for this tool
        QString binary;         // name of the executable (may contain path)
        QString args;           // any arguments
        QString shortcut;       // keyboard shortcut in QKeySequence format
        QIcon   icon;           // associated icon (if any)
        int     id;             // unique tool id
    };

    
    // newsserver account. currently doesnt contain user sensitive
    // information so therefore the specified server is always matched
    // to the list of configured servers in the hostapp
    struct account
    {
        QString name;
    };

    struct article
    {
        QString        name;    // name for this download (human readable)
        QList<QString> groups;  // newsgroups that contain the article parts
        QList<QString> ids;     // message-ids or numbers for all parts that comprise this download
        unsigned int   bytes;   // message size in bytes
    };

    class plugin;

    class newsflash
    {
    public:
        virtual ~newsflash() {}

        // request the host application to exit
        // returns true if the request is honoured, otherwise false
        virtual bool exit() = 0;

        // show/hide the host application window.
        virtual void show(bool val) = 0;

        // show a message in the host application's status bar
        virtual void show_message(const QString& message) = 0;

        // Request host application to open and focus a certain
        // tab in the application main tab. This function returns immediately
        // and the actual focusing happens later.
        virtual void show_tab(const QString& tab_name) = 0;

        // Request host application to open settings dialog and active
        // the tab page with the specified name.
        // This function returns immediately and the actual settings dialog
        // is opened later.
        virtual void show_settings(const QString& default_page_name) = 0;

        // Write a message to host log.
        // Context should be the context from which the message originates, 
        // e.g. "myextension". 
        // message is the actual message string.
        virtual void write_log(log event, const QString& context, const QString& message) = 0;

        // Execute an external command through host application. This function returns true
        // if execution was sucesful. Otherwise it returns false.
        // This function returns immediately and leaves the new process running.
        virtual bool execute_command(const QString& command, const QStringList& args) = 0;
        
        // Execute a tool configured in the system. This function returns true if execution 
        // was succesful. Otherwise it returns false.
        // This function returns immediately and leaves the new process runnig.
        virtual bool execute_tool(int tool_id, const QString& file) = 0;
        
        // Recommend a new tool for addition to the collection of tools. Whether a tool
        // is accepted depends on host policy, whether the tool already exists etc.
        // Returns true if the tool is added, false if not.
        virtual bool recommend_tool(const QString& binary, int filetypes, const QString& name = QString(), const QString& args = "${file}") = 0;

        // Try to open the specified resource (folder, file...) with generic
        // platform specific open methods. 
        virtual bool open(const QString& resource) = 0;

        // Try to match the given filename (with path or without path) against
        // the file type patterns configured in the host application.
        // If no match is made -1 is returned, otherwise the matching filetype
        // enum value is returned.
        virtual int match_filetype(const QString& filename) const = 0;

        // Try to name the given filetype and return a human readable 
        // name describing the filetypem
        virtual QString name_filetype(filetype type) const = 0;

        // Request an icon for the specified filetype.
        virtual QIcon icon_filetype(filetype type) const = 0;

        // Try to find a filename in the given subject line.
        // For example if subject line is "(6/28) nGJ6001.JPG = foobar", "nGJ6001.JPG" 
        // is returned. If no filename is found then an empty string is returned.
        virtual QString find_filename(const QString& subject) const = 0;

        // Try to suggest a name based on the list of subject lines. The name can 
        // be used as a suggested download folder name. If nothing reasonable
        // can be suggested an emptry string is returned.
        //virtual QString suggest_location(const QStringList& subjects) const = 0;

        // Open up a dialog for editing the suggestion and the path (or optionally browsing)
        // for a new path. The edited data is saved in suggestion and path.
        // Function returns true if changes were done otherwise (on user cancel)
        // it returns false.
        //virtual bool edit_suggestion(QString& suggestion, QString& path) const = 0;

        // Select one of the configured Usenet services. If there are no services
        // configured this function will prompt the user for input to create a new service.
        // If there are several services and the preferred service exists it is selected 
        // and user interaction is not required. If preferred is empty or is not available
        // any longer then user is asked to choose one of the configured services.
        // If interaction is canceled an empty string is returned. On succesful operation
        // a service name naming the chosen service is returned.
        // Optional download_name is a hint for the user to understand the context 
        // of the download
        virtual QString choose_service(const QString& download_name = QString()) = 0;


        // Download the articles in the items list from the specified host.
        // The downloaded data is placed in download_dir which is created if
        // it doesn't exist
        virtual bool download(const sdk::account& account, const QString& download_dir, const QString& description, const QList<article>& items, bool enable_fill = true) = 0;

        // Get the list of plugins providing the requested features
        virtual QList<plugin*> get_plugins(unsigned features) = 0;
        
        // Runtime resources ///
        


        // Deletes a persistent property permanently.
        virtual void del_property(const QString& context, const QString& name) = 0;
                
        // Set a persistent property. Context should be the extension context for example
        // "my-extension". Name is any name that is unique within the context.
        virtual void set_property(const QString& context, const QString& name, const QVariant& value) = 0;
        
        // Get a persistent property. Context should be the extension specified context
        // for example "my-extension". Name is any name that is unique within the context.
        virtual QVariant get_property(const QString& context, const QString& name) const = 0;

        virtual QVariant get_setting(settings setting) const = 0;

        // Get a list of tools configured for this particular filetype category.
        virtual QList<tool> get_tools(filetype category) const = 0;
        
        // Get a list of tools configured for THESE categories.
        virtual QList<tool> get_tools(int categories) const = 0;
        
        // Get a list of all all tools configured in the host app.
        virtual QList<tool> get_tools() const = 0;
        
        // Get a list of configured accounts
        virtual QList<account> get_accounts() const = 0;
        
        // Get application name (title)
        virtual QString get_title() const = 0;
        
        // Get application version string
        virtual QString get_version() const = 0;

        // Get the list of latest download paths
        virtual QStringList get_download_paths() const = 0;

        // Add a path that was used for downloading to the
        // application's download paths.
        virtual void add_download_path(const QString& path) = 0;

        // Get/set latest NZB file path for saving (and loading?)
        virtual void set_nzbpath(const QString& path) = 0;
        virtual QString get_nzbpath() const = 0;
    };

    // get the most recent download path. This is a convenience
    // function and is same as getting the tail of the QStringList
    // returned by get_download_paths
    inline
    QString get_download_path(const newsflash* app)
    {
        const QStringList& list = app->get_download_paths();
        if (list.isEmpty())
            return QString();
        return list.last();
    }

} // sdk



