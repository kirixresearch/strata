/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2011-08-01
 *
 */


#include "appmain.h"


void apphookInitFrame()
{
}

void apphookInitCommands()
{
}

void apphookInitMenuBar(wxMenuBar* menubar)
{
}

void apphookAppStartup()
{
}

void apphookAppReady()
{
}

void apphookPostOpenProject()
{
}

bool apphookOpenWeb(const wxString& location, bool* handled)
{
    return true;
}

bool apphookOpenTemplate(const wxString& location,
                         xd::IFileInfoPtr file_info,
                         bool* handled)
{
    wxASSERT(handled);
    *handled = false;
    return true;
}

jobs::IJobPtr apphookExecute(const wxString& path,
                             xd::IFileInfoPtr file_info,
                             bool* handled)
{
    wxASSERT(handled);
    *handled = false;
    return xcm::null;
}

void apphookUninitFrame()
{
}

void apphookOnExit()
{
}

