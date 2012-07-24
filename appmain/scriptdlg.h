/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-06-20
 *
 */


#ifndef __APP_SCRIPTDLG_H
#define __APP_SCRIPTDLG_H


#if wxCHECK_VERSION(2,9,0)
#include <wx/colourdata.h>
#endif

#include "scriptgui.h"


class FileDialog : public FormControl
{
    BEGIN_KSCRIPT_CLASS("FileDialog", FileDialog)
        KSCRIPT_GUI_METHOD("constructor", FileDialog::constructor)
        KSCRIPT_GUI_METHOD("setDirectory", FileDialog::setDirectory)        
        KSCRIPT_GUI_METHOD("getDirectory", FileDialog::getDirectory)
        KSCRIPT_GUI_METHOD("setCaption", FileDialog::setCaption)
        KSCRIPT_GUI_METHOD("getCaption", FileDialog::getCaption)
        KSCRIPT_GUI_METHOD("setFilter", FileDialog::setFilter)
        KSCRIPT_GUI_METHOD("getFilter", FileDialog::getFilter)
        KSCRIPT_GUI_METHOD("setDefaultFilter", FileDialog::setDefaultFilter)
        KSCRIPT_GUI_METHOD("getDefaultFilter", FileDialog::getDefaultFilter)
        KSCRIPT_GUI_METHOD("getFilename", FileDialog::getFilename)
        KSCRIPT_GUI_METHOD("getFilenames", FileDialog::getFilenames)
        KSCRIPT_GUI_METHOD("getPath", FileDialog::getPath)
        KSCRIPT_GUI_METHOD("getPaths", FileDialog::getPaths)
        KSCRIPT_GUI_METHOD("showDialog", FileDialog::showDialog)
    END_KSCRIPT_CLASS()

public:

    FileDialog();
    ~FileDialog();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void getDirectory(kscript::ExprEnv* env, kscript::Value* retval);
    void setDirectory(kscript::ExprEnv* env, kscript::Value* retval);
    void getCaption(kscript::ExprEnv* env, kscript::Value* retval);
    void setCaption(kscript::ExprEnv* env, kscript::Value* retval);
    void getFilter(kscript::ExprEnv* env, kscript::Value* retval);
    void setFilter(kscript::ExprEnv* env, kscript::Value* retval);
    void getDefaultFilter(kscript::ExprEnv* env, kscript::Value* retval);
    void setDefaultFilter(kscript::ExprEnv* env, kscript::Value* retval);
    void getFilename(kscript::ExprEnv* env, kscript::Value* retval);
    void getFilenames(kscript::ExprEnv* env, kscript::Value* retval);
    void getPath(kscript::ExprEnv* env, kscript::Value* retval);
    void getPaths(kscript::ExprEnv* env, kscript::Value* retval);
    void showDialog(kscript::ExprEnv* env, kscript::Value* retval);

protected:

    wxString m_filter;
    wxString m_message;         // this is actually the caption
    wxString m_default_dir;
    int m_filter_index;

    bool m_open;
    bool m_multiselect;
    wxArrayString m_paths;
    wxArrayString m_filenames;
};


class OpenFileDialog : public FileDialog
{
    BEGIN_KSCRIPT_DERIVED_CLASS("OpenFileDialog", OpenFileDialog, FileDialog)
        KSCRIPT_GUI_METHOD("constructor", OpenFileDialog::constructor)
        KSCRIPT_GUI_METHOD("setMultiselect", OpenFileDialog::setMultiselect)
    END_KSCRIPT_CLASS()

public:

    OpenFileDialog();
    ~OpenFileDialog();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setMultiselect(kscript::ExprEnv* env, kscript::Value* retval);
};


class SaveFileDialog : public FileDialog
{
    BEGIN_KSCRIPT_DERIVED_CLASS("SaveFileDialog", SaveFileDialog, FileDialog)
        KSCRIPT_GUI_METHOD("constructor", SaveFileDialog::constructor)
    END_KSCRIPT_CLASS()

public:

    SaveFileDialog();
    ~SaveFileDialog();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
};


class DirectoryDialog : public FormControl
{
    BEGIN_KSCRIPT_CLASS("DirectoryDialog", DirectoryDialog)
        KSCRIPT_GUI_METHOD("constructor", DirectoryDialog::constructor)
        KSCRIPT_GUI_METHOD("setPath", DirectoryDialog::setPath)
        KSCRIPT_GUI_METHOD("getPath", DirectoryDialog::getPath)
        KSCRIPT_GUI_METHOD("setMessage", DirectoryDialog::setMessage)
        KSCRIPT_GUI_METHOD("getMessage", DirectoryDialog::getMessage)
        KSCRIPT_GUI_METHOD("showDialog", DirectoryDialog::showDialog)
    END_KSCRIPT_CLASS()

public:

    DirectoryDialog();
    ~DirectoryDialog();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setPath(kscript::ExprEnv* env, kscript::Value* retval);
    void getPath(kscript::ExprEnv* env, kscript::Value* retval);
    void setMessage(kscript::ExprEnv* env, kscript::Value* retval);
    void getMessage(kscript::ExprEnv* env, kscript::Value* retval);
    void showDialog(kscript::ExprEnv* env, kscript::Value* retval);

protected:

    wxString m_message;
    wxString m_path;
};


class TextEntryDialog : public FormControl
{
    BEGIN_KSCRIPT_CLASS("TextEntryDialog", TextEntryDialog)
        KSCRIPT_GUI_METHOD("constructor", TextEntryDialog::constructor)
        KSCRIPT_GUI_METHOD("setText", TextEntryDialog::setText)
        KSCRIPT_GUI_METHOD("getText", TextEntryDialog::getText)        
        KSCRIPT_GUI_METHOD("setMessage", TextEntryDialog::setMessage)
        KSCRIPT_GUI_METHOD("getMessage", TextEntryDialog::getMessage)        
        KSCRIPT_GUI_METHOD("setCaption", TextEntryDialog::setCaption)
        KSCRIPT_GUI_METHOD("getCaption", TextEntryDialog::getCaption)
        KSCRIPT_GUI_METHOD("showDialog", TextEntryDialog::showDialog)
    END_KSCRIPT_CLASS()

public:

    TextEntryDialog();
    ~TextEntryDialog();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setText(kscript::ExprEnv* env, kscript::Value* retval);
    void setMessage(kscript::ExprEnv* env, kscript::Value* retval);
    void setCaption(kscript::ExprEnv* env, kscript::Value* retval);
    void getText(kscript::ExprEnv* env, kscript::Value* retval);
    void getMessage(kscript::ExprEnv* env, kscript::Value* retval);
    void getCaption(kscript::ExprEnv* env, kscript::Value* retval);
    void showDialog(kscript::ExprEnv* env, kscript::Value* retval);
    
protected:

    long m_style;
    wxString m_caption;
    wxString m_message;
    wxString m_text;
};


class PasswordEntryDialog : public TextEntryDialog
{
    BEGIN_KSCRIPT_DERIVED_CLASS("PasswordEntryDialog", PasswordEntryDialog, TextEntryDialog)
        KSCRIPT_GUI_METHOD("constructor", PasswordEntryDialog::constructor)
    END_KSCRIPT_CLASS()

public:

    PasswordEntryDialog();
    ~PasswordEntryDialog();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
};


class ColorDialog : public FormControl
{
    BEGIN_KSCRIPT_CLASS("ColorDialog", ColorDialog)
        KSCRIPT_GUI_METHOD("constructor", ColorDialog::constructor)
        KSCRIPT_GUI_METHOD("setColor", ColorDialog::setColor)
        KSCRIPT_GUI_METHOD("getColor", ColorDialog::getColor)
        KSCRIPT_GUI_METHOD("setCaption", ColorDialog::setCaption)
        KSCRIPT_GUI_METHOD("getCaption", ColorDialog::getCaption)
        KSCRIPT_GUI_METHOD("setCustomColor", ColorDialog::setCustomColor)
        KSCRIPT_GUI_METHOD("getCustomColor", ColorDialog::getCustomColor)
        KSCRIPT_GUI_METHOD("showAllColors", ColorDialog::showAllColors)
        KSCRIPT_GUI_METHOD("showDialog", ColorDialog::showDialog)
    END_KSCRIPT_CLASS()

public:

    ColorDialog();
    ~ColorDialog();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setCaption(kscript::ExprEnv* env, kscript::Value* retval);
    void setColor(kscript::ExprEnv* env, kscript::Value* retval);
    void setCustomColor(kscript::ExprEnv* env, kscript::Value* retval);
    void getCaption(kscript::ExprEnv* env, kscript::Value* retval);
    void getColor(kscript::ExprEnv* env, kscript::Value* retval);
    void getCustomColor(kscript::ExprEnv* env, kscript::Value* retval);
    void showAllColors(kscript::ExprEnv* env, kscript::Value* retval);
    void showDialog(kscript::ExprEnv* env, kscript::Value* retval);

protected:

    wxColourData m_color_data;
    wxString m_caption;
};


class ProjectFileDialog : public FormControl
{
public:

    BEGIN_KSCRIPT_CLASS("ProjectFileDialog", ProjectFileDialog)

        KSCRIPT_GUI_METHOD("constructor", ProjectFileDialog::constructor)
        KSCRIPT_GUI_METHOD("setPath", ProjectFileDialog::setPath)
        KSCRIPT_GUI_METHOD("getPath", ProjectFileDialog::getPath)        
        KSCRIPT_GUI_METHOD("setCaption", ProjectFileDialog::setCaption)
        KSCRIPT_GUI_METHOD("getCaption", ProjectFileDialog::getCaption)
        KSCRIPT_GUI_METHOD("showDialog", ProjectFileDialog::showDialog)

        KSCRIPT_CONSTANT_INTEGER("OpenFile",     OpenFile)
        KSCRIPT_CONSTANT_INTEGER("SaveFile",     SaveFile)
        KSCRIPT_CONSTANT_INTEGER("ChooseFolder", ChooseFolder)

    END_KSCRIPT_CLASS()

public:

    enum
    {
        OpenFile = 0,
        SaveFile,
        ChooseFolder
    };

public:

    ProjectFileDialog();
    ~ProjectFileDialog();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setPath(kscript::ExprEnv* env, kscript::Value* retval);
    void setCaption(kscript::ExprEnv* env, kscript::Value* retval);
    void getPath(kscript::ExprEnv* env, kscript::Value* retval);
    void getCaption(kscript::ExprEnv* env, kscript::Value* retval);
    void showDialog(kscript::ExprEnv* env, kscript::Value* retval);
    
private:

    int m_mode;
    wxString m_path;
    wxString m_caption;
    bool m_caption_set;
};

/*
class PrintDialog : public FormControl
{
    BEGIN_KSCRIPT_CLASS("PrintDialog", PrintDialog)
        KSCRIPT_GUI_METHOD("constructor", PrintDialog::constructor)
        KSCRIPT_GUI_METHOD("enableHelp", PrintDialog::enableHelp)
        KSCRIPT_GUI_METHOD("enablePageNumbers", PrintDialog::enablePageNumbers)
        KSCRIPT_GUI_METHOD("enablePrintToFile", PrintDialog::enablePrintToFile)
        KSCRIPT_GUI_METHOD("enableSelections", PrintDialog::enableSelections)
        KSCRIPT_GUI_METHOD("getAllPages", PrintDialog::getAllPages)
        KSCRIPT_GUI_METHOD("getCollate", PrintDialog::getCollate)
        KSCRIPT_GUI_METHOD("getFromPage", PrintDialog::getFromPage)
        KSCRIPT_GUI_METHOD("getToPage", PrintDialog::getToPage)
        KSCRIPT_GUI_METHOD("getMaxPage", PrintDialog::getMaxPage)
        KSCRIPT_GUI_METHOD("getMinPage", PrintDialog::getMinPage)
        KSCRIPT_GUI_METHOD("getCopyCount", PrintDialog::getCopyCount)
        KSCRIPT_GUI_METHOD("getPrintToFile", PrintDialog::getPrintToFile)
        KSCRIPT_GUI_METHOD("getSelection", PrintDialog::getSelection)
        KSCRIPT_GUI_METHOD("setCollate", PrintDialog::setCollate)
        KSCRIPT_GUI_METHOD("setFromPage", PrintDialog::setFromPage)
        KSCRIPT_GUI_METHOD("setToPage", PrintDialog::setToPage)
        KSCRIPT_GUI_METHOD("setMaxPage", PrintDialog::setMaxPage)
        KSCRIPT_GUI_METHOD("setMinPage", PrintDialog::setMinPage)
        KSCRIPT_GUI_METHOD("setCopyCount", PrintDialog::setCopyCount)
        KSCRIPT_GUI_METHOD("setPrintToFile", PrintDialog::setPrintToFile)
        KSCRIPT_GUI_METHOD("setSelection", PrintDialog::setSelection)
        KSCRIPT_GUI_METHOD("showDialog", PrintDialog::)
    END_KSCRIPT_CLASS()

public:

    PrintDialog();
    ~PrintDialog();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void enableHelp(kscript::ExprEnv* env, kscript::Value* retval);
    void enablePageNumbers(kscript::ExprEnv* env, kscript::Value* retval);
    void enablePrintToFile(kscript::ExprEnv* env, kscript::Value* retval);
    void enableSelections(kscript::ExprEnv* env, kscript::Value* retval);
    
    void getAllPages(kscript::ExprEnv* env, kscript::Value* retval);
    void getCollate(kscript::ExprEnv* env, kscript::Value* retval);
    void getFromPage(kscript::ExprEnv* env, kscript::Value* retval);
    void getToPage(kscript::ExprEnv* env, kscript::Value* retval);
    void getMaxPage(kscript::ExprEnv* env, kscript::Value* retval);
    void getMinPage(kscript::ExprEnv* env, kscript::Value* retval);
    void getCopyCount(kscript::ExprEnv* env, kscript::Value* retval);
    void getPrintToFile(kscript::ExprEnv* env, kscript::Value* retval);
    void getSelection(kscript::ExprEnv* env, kscript::Value* retval);
    
    void setCollate(kscript::ExprEnv* env, kscript::Value* retval);
    void setFromPage(kscript::ExprEnv* env, kscript::Value* retval);
    void setToPage(kscript::ExprEnv* env, kscript::Value* retval);
    void setMaxPage(kscript::ExprEnv* env, kscript::Value* retval);
    void setMinPage(kscript::ExprEnv* env, kscript::Value* retval);
    void setCopyCount(kscript::ExprEnv* env, kscript::Value* retval);
    void setPrintToFile(kscript::ExprEnv* env, kscript::Value* retval);
    void setSelection(kscript::ExprEnv* env, kscript::Value* retval);

protected:

    wxPrintDialogData m_print_dialog_data;
    wxPrintData m_print_data;
};

*/


#endif

