/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Paladin Code Generator Client
 * Author:   Benjamin I. Williams
 * Created:  2003-04-27
 *
 */


#include <wx/wx.h>
#include <wx/cmdline.h>
#include "app.h"
#include "gencode.h"


#if wxUSE_GUI!=0
#include "mainframe.h"
#endif


#if wxUSE_GUI==0
IMPLEMENT_APP(PalGenApp)
#else
IMPLEMENT_APP(PalGenApp)
#endif


static const wxCmdLineEntryDesc cmdLineDesc[] =
{
    { wxCMD_LINE_OPTION, "a", "apptag",  "application tag", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_OPTION, "s", "sitecode", "site code", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_OPTION, "x", "valsitecode", "validate site code", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_OPTION, "e", "expdate",  "expiration date (yyyymmdd)", wxCMD_LINE_VAL_NUMBER, wxCMD_LINE_PARAM_OPTIONAL},
    { wxCMD_LINE_SWITCH, "c", "computerid", "generate a computer id" },
    { wxCMD_LINE_NONE }
};


static bool processCmdLine(int argc, wxChar* argv[])
{
    wxLogNull log;

    wxCmdLineParser parser(argc, argv);

    parser.SetDesc(cmdLineDesc);

    int res = parser.Parse(true);
    if (res != 0)
        return false;

    wxString apptag;
    wxString sitecode;
    wxString val_sitecode;
    wxString expdate;
    bool gen_auth = true;

    long l_expdate = 0;



    if (parser.Found(wxT("x"), &val_sitecode))
    {
        // validate site code
        printf("%s", validateSiteCode(val_sitecode) ? "valid" : "invalid");
        return true;
    }
    
    gen_auth = parser.Found(wxT("c")) ? false : true;

    if (!parser.Found(wxT("a"), &apptag) && gen_auth)
        return false;

    if (!parser.Found(wxT("e"), &l_expdate) && gen_auth)
        return false;

    if (!parser.Found(wxT("s"), &sitecode))
        return false;



    expdate.Printf(wxT("%08d"), l_expdate);

    int exp_year = wxAtoi(expdate.Left(4));
    int exp_month = wxAtoi(expdate.Mid(4,2));
    int exp_day = wxAtoi(expdate.Mid(6,2));


    if (gen_auth)
    {
        printf("%s", (const char*)calcActivationCode(apptag,
                                                 sitecode,
                                                 0,
                                                 exp_year,
                                                 exp_month,
                                                 exp_day).mbc_str());
    }
     else
    {
        printf("%s", (const char*)calcComputerId(sitecode).mbc_str());
    }

    return true;
}


bool PalGenApp::OnInit()
{
#if wxUSE_GUI!=0
    if (argc <= 1)
    {
        MainFrame* frame = new MainFrame;
        frame->Show(TRUE);
        return true;
    }
#endif

    processCmdLine(argc, argv);

    return false;
}


#if wxUSE_GUI==0
int PalGenApp::OnRun()
{
    processCmdLine(argc, argv);
}
#endif

int PalGenApp::OnExit()
{
    return 0;
}


