/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Paladin Code Generator Client
 * Author:   Benjamin I. Williams
 * Created:  2003-04-27
 *
 */


#include <wx/wx.h>
#include <wx/calctrl.h>
#include <wx/notebook.h>
#include "app.h"
#include "infopage.h"
#include "generatepage.h"
#include "mainframe.h"


enum
{
    ID_Generate = wxID_HIGHEST+1,
    ID_Quit,
    ID_About,
    ID_SiteCodeInfo,
    ID_LicenseKeyInfo,

};


BEGIN_EVENT_TABLE(MainFrame, wxFrame)
END_EVENT_TABLE()


MainFrame::MainFrame() : wxFrame(NULL, -1, _("Paladin Code Generator"),
                                 wxPoint(100,100), wxSize(700,540), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxCLIP_CHILDREN | wxNO_FULL_REPAINT_ON_RESIZE)
{
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);

    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    // -- make a menu --

    wxMenu* menuFile = new wxMenu;
    menuFile->Append(ID_SiteCodeInfo, _("Get Site Code Info..."));
    menuFile->Append(ID_LicenseKeyInfo, _("Get License Key Info..."));
    menuFile->AppendSeparator();
    menuFile->Append(ID_Quit, _("Exit"));

    wxMenu* menuHelp = new wxMenu;
    menuHelp->Append(ID_About, _("&About Paladin Code Generator..."));

    wxMenuBar* menuBar = new wxMenuBar;
    menuBar->Append(menuFile, _("&File"));
    menuBar->Append(menuHelp, _("&Help"));


    SetMenuBar(menuBar);


    wxNotebook* notebook = new wxNotebook(this, -1);
    notebook->AddPage(new GenerationPage(notebook), _("Key Generation"));
    notebook->AddPage(new InfoPage(notebook), _("Code Information"));

    main_sizer->Add(notebook, 1, wxEXPAND);

    SetSizer(main_sizer);
}

MainFrame::~MainFrame()
{
}


