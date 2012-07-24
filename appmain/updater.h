/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-11-08
 *
 */


#ifndef __APP_UPDATER_H
#define __APP_UPDATER_H


struct UpdaterInfo
{
    wxString fetch_url;
    wxString user_name;
    wxString password;
};




class Updater
{
public:

    Updater();
    virtual ~Updater();
    
    // this function is called when "Check for Updates" is selected
    // from the menu
    static void checkForUpdates(bool full_gui = true);

public:

    static void showNoUpdates();
    static bool showAskForUpdate();
    
    static bool parseUpdateFile(const wxString& xml,
                                UpdaterInfo& info);
};


#endif

