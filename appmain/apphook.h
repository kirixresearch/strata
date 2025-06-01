/*!
 *
 * Copyright (c) 2011-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2011-07-21
 *
 */


#ifndef H_APP_APPHOOK_H
#define H_APP_APPHOOK_H


void apphookInitFrame();
void apphookInitCommands();
void apphookInitMenuBar(wxMenuBar* menubar);
void apphookInitPanels();
void apphookAppStartup();
void apphookAppReady();

void apphookPostOpenProject();
void apphookLicenseInstalled();
void apphookPostLicenseCheck();



bool apphookOpenTemplate(const wxString& location,
                         xd::IFileInfoPtr file_info,
                         bool* handled);

bool apphookOpenWeb(const wxString& location,
                    bool* handled);

jobs::IJobPtr apphookExecute(const wxString& location,
                             xd::IFileInfoPtr file_info,
                             bool* handled);

void apphookUninitFrame();
void apphookOnExit();


#endif
