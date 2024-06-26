/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2001-08-09
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

