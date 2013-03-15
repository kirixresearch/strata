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

bool apphookOpenTemplate(const wxString& location,
                         tango::IFileInfoPtr file_info,
                         bool* handled)
{
    wxASSERT(handled);
    *handled = false;
    return true;
}


IJobPtr apphookExecute(const wxString& path,
                            tango::IFileInfoPtr file_info,
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

