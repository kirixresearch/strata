/*!
 *
 * Copyright (c) 2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2011-07-21
 *
 */

#ifndef __APP_APPHOOK_H
#define __APP_APPHOOK_H




void apphookInitFrame();
void apphookInitCommands();
void apphookInitMenuBar(wxMenuBar* menubar);

bool apphookOpenTemplate(const wxString& location,
                         tango::IFileInfoPtr file_info,
                         tango::INodeValuePtr file_root,
                         bool* handled);
                         
IJobPtr apphookExecute(const wxString& location,
                            tango::IFileInfoPtr file_info,
                            tango::INodeValuePtr file_root,
                            bool* handled);

void apphookUninitFrame();
void apphookOnExit();


#endif
