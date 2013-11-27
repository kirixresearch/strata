
#include <wx/wx.h>
#include "app.h"
#include "mainframe.h"

IMPLEMENT_APP(SdconnApp)

bool SdconnApp::OnInit()
{
    MainFrame *frame = new MainFrame(_T("SD Connector"), wxPoint(50,50),
                wxSize(408,350));

    frame->Show(TRUE);
    SetTopWindow(frame);
    return TRUE;
}