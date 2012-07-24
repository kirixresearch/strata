/*!
 *
 * Copyright (c) 2001-2011, Kirix Research, LLC.  All rights reserved.
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
                         tango::IFileInfoPtr file_info,
                         tango::INodeValuePtr file_root,
                         bool* handled)
{
    wxASSERT(handled);
    *handled = false;
    return true;
}


cfw::IJobPtr apphookExecute(const wxString& path,
                            tango::IFileInfoPtr file_info,
                            tango::INodeValuePtr file_root,
                            bool* handled)
{
    wxASSERT(handled);
    *handled = false;
    return xcm::null;
}

