/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-12-24
 *
 */


#include "appmain.h"
#include "dlgdatabasefile.h"
#include "scriptgui.h"
#include "scriptdlg.h"
#include <wx/colordlg.h>


// -- FileDialog class implementation --

// (CLASS) FileDialog
// Category: Dialog
// Description: A class that represents a file chooser dialog.
// Remarks: Class that represents a file chooser dialog.

FileDialog::FileDialog()
{
    wxString filter = _("All Files");
    filter += wxT(" (*.*)|*.*|");
    filter.RemoveLast(); // get rid of the last pipe sign
    
    m_open = true;
    m_multiselect = true;
    m_default_dir = wxEmptyString;
    m_message = _("Choose File");
    m_filter = filter;
    m_filter_index = 0;
}

FileDialog::~FileDialog()
{
}

// (CONSTRUCTOR) FileDialog.constructor
// Description: Creates a new FileDialog.
//
// Syntax: FileDialog()
//
// Remarks: Creates a new FileDialog.

void FileDialog::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

// (METHOD) FileDialog.setDirectory
// Description: Sets the default directory of the file dialog.
//
// Syntax: function FileDialog.setDirectory(path : String)
//
// Remarks: Sets the default directory of the file dialog.
//
// Param(path): The new default directory |path| for the file dialog.

void FileDialog::setDirectory(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_default_dir = towx(env->getParam(0)->getString());
}

// (METHOD) FileDialog.getDirectory
// Description: Gets the default directory of the file dialog.
//
// Syntax: function FileDialog.getDirectory() : String
//
// Remarks: Returns the default directory of the file dialog.
//
// Returns: Returns the default directory of the file dialog.

void FileDialog::getDirectory(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setString(towstr(m_default_dir));
}

// (METHOD) FileDialog.setCaption
// Description: Sets the caption to display in the file dialog.
//
// Syntax: function FileDialog.setCaption(text : String)
//
// Remarks: Sets the caption to display in the file dialog.
//
// Param(text): The |text| to display as the caption in the file dialog.

void FileDialog::setCaption(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_message = towx(env->getParam(0)->getString());
}

// (METHOD) FileDialog.getCaption
// Description: Gets the caption that is displayed in the file dialog.
//
// Syntax: function FileDialog.getCaption() : String
//
// Remarks: Returns the caption to display in the file dialog.
//
// Returns: Returns the caption to display in the file dialog.

void FileDialog::getCaption(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setString(towstr(m_message));
}

// (METHOD) FileDialog.setFilter
// Description: Sets the filter string for the file dialog.
//
// Syntax: function FileDialog.setFilter(text : String)
//
// Remarks: Sets the filter string for the file dialog.  A filter string
//     is composed of one or more description/extension pairs.  The file dialog
//     will allow the user to search for files with the type(s) specified in the
//     filter string.  A filter string for .bmp and .jpg/.jpeg files might look
//     something like this: "Bitmap Files|*.bmp|JPEG Images|*.jpg;*.jpeg"

void FileDialog::setFilter(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_filter = towx(env->getParam(0)->getString());
}

// (METHOD) FileDialog.getFilter
// Description: Gets the filter string for the file dialog.
//
// Syntax: function FileDialog.getFilter() : String
//
// Remarks: Returns the filter string for the file dialog.
//
// Returns: Returns a string containing the current filter string
//     for the file dialog.

void FileDialog::getFilter(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setString(towstr(m_filter));
}

// (METHOD) FileDialog.getDefaultFilter
// Description: Gets the default filter index for the file dialog.
//
// Syntax: function FileDialog.getDefaultFilter() : Integer
//
// Remarks: Returns the default filter index for the file dialog.
//
// Returns: Returns the default filter index for the file dialog.

void FileDialog::getDefaultFilter(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setInteger(m_filter_index);
}

// (METHOD) FileDialog.setDefaultFilter
// Description: Sets the default filter index for the file dialog.
//
// Syntax: function FileDialog.setDefaultFilter(index : Integer)
//
// Remarks: Sets the default filter to the value specified in the
//     |index| parameter.  This zero-based index indicates which filter
//     string is shown by default.  See the setFilter() method for more
//     information.
//
// Param(index): The default |index| to which to set the file dialog filter.

void FileDialog::setDefaultFilter(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_filter_index = env->getParam(0)->getInteger();
}

// (METHOD) FileDialog.getFilename
// Description: Gets the file name from the file dialog.
//
// Syntax: function FileDialog.getFilename() : String
//
// Remarks: Returns the file name from the file dialog.
//
// Returns: Returns the file name from the file dialog.

void FileDialog::getFilename(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_filenames.GetCount() == 0)
    {
        retval->setString(L"");
        return;
    }

    retval->setString(towstr(m_filenames.Item(0)));
}

// (METHOD) FileDialog.getFilenames
// Description: Returns an array of file names from the file dialog.
//
// Syntax: function FileDialog.getFilenames() : Array(String)
//
// Remarks: Returns an array of the selected files in the file dialog.
//          In order for the file dialog to return multiple paths,
//          multi-selection mode must be enabled.
//          See OpenFileDialog.setMultiselect()
//
// Returns: Returns an array of the selected files in the file dialog.

void FileDialog::getFilenames(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setArray(env);

    int count = m_filenames.Count();
    int i;

    for (i = 0; i < count; ++i)
    {
        kscript::Value val;
        val.setString(towstr(m_filenames.Item(i)));
        retval->appendMember(&val);
    }
}

// (METHOD) FileDialog.getPath
// Description: Gets the file name of the selected file, including the full directory path.
//
// Syntax: function FileDialog.getPath() : String
//
// Remarks: Returns the name of the selected file, including the full directory path.  If
//     multiple files were selected in the file dialog, the first one will be returned.
//
// Returns: Returns the name of the selected file, including the full directory path.

void FileDialog::getPath(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_paths.GetCount() == 0)
    {
        retval->setString(L"");
        return;
    }

    retval->setString(towstr(m_paths.Item(0)));
}

// (METHOD) FileDialog.getPaths
// Description: Returns an array of the file names of the selected files 
//     that includes the full directory path.
//
// Syntax: function FileDialog.getPaths() : Array(String)
//
// Remarks: Returns an array of the file names of the selected files
//     that includes the full directory path.  In order for the file
//     dialog to return multiple paths, multi-selection mode must
//     be enabled.  See OpenFileDialog.setMultiselect()
//
// Returns: Returns an array of the file names of the selected files
//     that includes the full directory path.

void FileDialog::getPaths(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setArray(env);

    int count = m_paths.Count();
    int i;

    for (i = 0; i < count; ++i)
    {
        kscript::Value val;
        val.setString(towstr(m_paths.Item(i)));
        retval->appendMember(&val);
    }
}

// (METHOD) FileDialog.showDialog
// Description: Shows the file dialog.
//
// Syntax: function FileDialog.showDialog() : DialogResult
//
// Remarks: Shows a file dialog and returns DialogResult.Ok if the user
//      closes the file dialog by pressing "OK", or the equivalent, and
//      DialogResult.Cancel if the user closes the file dialog by pressing
//      "Cancel", or the equivalent.
//
// Returns: Returns DialogResult.Ok when the dialog is closed by pressing
//      "OK" or equivalent, and DialogResult.Cancel if the dialog is closed
//      by pressing "Cancel" or equivalent.

void FileDialog::showDialog(kscript::ExprEnv* env, kscript::Value* retval)
{
    unsigned int flags = 0;

    if (m_open)
        flags |= wxFD_OPEN;
         else
        flags |= wxFD_SAVE;
    
    if (m_open && m_multiselect)
        flags |= wxFD_MULTIPLE;

    m_paths.Clear();

    wxFileDialog dlg(g_app->getMainWindow(),
                     m_message,
                     m_default_dir,
                     wxEmptyString,
                     m_filter,
                     flags);

    dlg.SetFilterIndex(m_filter_index);

    int res = dlg.ShowModal();

    if (res != wxID_OK)
    {
        retval->setInteger(DialogResult::Cancel);
        return;
    }

    if (m_multiselect)
    {
        dlg.GetPaths(m_paths);
        dlg.GetFilenames(m_filenames);
    }
     else
    {
        m_paths.Add(dlg.GetPath());
        m_filenames.Add(dlg.GetFilename());
    }

    retval->setBoolean(DialogResult::Ok);
}




// -- OpenFileDialog class implementation --

// (CLASS) OpenFileDialog
// Category: Dialog
// Derives: FileDialog
// Description: A class that represents a file open dialog.
// Remarks: Class that represents a file open dialog.

OpenFileDialog::OpenFileDialog() : FileDialog()
{
    m_open = true;
}

OpenFileDialog::~OpenFileDialog()
{
}

// (CONSTRUCTOR) OpenFileDialog.constructor
// Description: Creates a new OpenFileDialog.
//
// Syntax: OpenFileDialog()
//
// Remarks: Creates a new OpenFileDialog.

void OpenFileDialog::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    FileDialog::constructor(env, retval);
}

// (METHOD) OpenFileDialog.setMultiselect
// Description: Sets whether or not multiple selections are allowed.
//
// Syntax: OpenFileDialog.setMultiselect()
// Syntax: OpenFileDialog.setMultiselect(flag : Boolean)
//
// Remarks: Sets whether or not multiple selections are allowed. If |flag|
//     is true, multiple selections are allowed. If |flag| is false, multiple
//     selections are not allowed. If |flag| is not specified, then multiple
//     selections are allowed.
//
// Param(flag): A |flag| that indicates whether or not multiple selections are
//     allowed.

void OpenFileDialog::setMultiselect(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() > 0)
    {
        m_multiselect = env->getParam(0)->getBoolean();
    }
     else
    {
        m_multiselect = true;
    }
}


// -- SaveFileDialog class implementation --

// (CLASS) SaveFileDialog
// Category: Dialog
// Derives: FileDialog
// Description: A class that represents a file save dialog.
// Remarks: Class that represents a file save dialog.

SaveFileDialog::SaveFileDialog() : FileDialog()
{
    m_open = false;
}

SaveFileDialog::~SaveFileDialog()
{
}

// (CONSTRUCTOR) SaveFileDialog.constructor
// Description: Creates a new SaveFileDialog.
//
// Syntax: SaveFileDialog()
//
// Remarks: Creates a new SaveFileDialog.

void SaveFileDialog::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    FileDialog::constructor(env, retval);
}




// -- DirectoryDialog class implementation --

// (CLASS) DirectoryDialog
// Category: Dialog
// Description: A class that represents a directory chooser dialog.
// Remarks: Class that represents a directory chooser dialog.

DirectoryDialog::DirectoryDialog()
{
    m_message = _("Choose a directory");
    m_path = wxEmptyString;
}

DirectoryDialog::~DirectoryDialog()
{

}

// (CONSTRUCTOR) DirectoryDialog.constructor
// Description: Creates a new DirectoryDialog.
//
// Syntax: DirectoryDialog()
//
// Remarks: Creates a new DirectoryDialog.

// NOTE: Sadly, as much as we'd like to add setCaption() and getCaption()
//       functions to this binding for consistency and functionality
//       purposes, the wxWidgets class doesn't allow the caption to be
//       set -- SetTitle() doesn't do anything

void DirectoryDialog::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

// (METHOD) DirectoryDialog.setPath
// Description: Sets the initial path to show in the directory dialog.
//
// Syntax: function DirectoryDialog.setPath(text : String)
//
// Remarks: Sets the initial path to show in the directory dialog.
//
// Param(text): The initial path to show in the directory dialog.

void DirectoryDialog::setPath(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_path = towx(env->getParam(0)->getString());
}

// (METHOD) DirectoryDialog.getPath
// Description: Gets the path selected by the user.
//
// Syntax: function DirectoryDialog.getPath() : String
//
// Remarks: Returns the path selected by the user.
//
// Returns: Returns the path selected by the user.

void DirectoryDialog::getPath(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setString(towstr(m_path));
}

// (METHOD) DirectoryDialog.setMessage
// Description: Sets the message to display in the directory dialog.
//
// Syntax: function DirectoryDialog.setMessage(text : String)
//
// Remarks: Sets the message to display in the directory dialog to |text|.
//
// Param(text): Sets the message to display in the directory dialog to |text|.

void DirectoryDialog::setMessage(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_message = towx(env->getParam(0)->getString());
}

// (METHOD) DirectoryDialog.getMessage
// Description: Gets the message to display in the directory dialog.
//
// Syntax: function DirectoryDialog.getMessage() : String
//
// Remarks: Returns the message to display in the directory dialog.
//
// Returns: Returns the message to display in the directory dialog.

void DirectoryDialog::getMessage(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setString(towstr(m_message));
}

// (METHOD) DirectoryDialog.showDialog
// Description: Shows the directory dialog.
//
// Syntax: function DirectoryDialog.showDialog() : Boolean
//
// Remarks: Shows a directory dialog and returns DialogResult.Ok if the user
//      closes the directory dialog by pressing "OK", or the equivalent, and
//      DialogResult.Cancel if the user closes the directory dialog by
//      pressing "Cancel", or the equivalent.
//
// Returns: Returns DialogResult.Ok when the dialog is closed by pressing
//      "OK" or equivalent, and DialogResult.Cancel if the dialog is closed
//      by pressing "Cancel" or equivalent.

void DirectoryDialog::showDialog(kscript::ExprEnv* env, kscript::Value* retval)
{
    wxDirDialog dlg(g_app->getMainWindow(),
                    m_message,
                    m_path);
    
    int res = dlg.ShowModal();
    if (res != wxID_OK)
    {
        retval->setInteger(DialogResult::Cancel);
        return;
    }

    m_path = dlg.GetPath();
    retval->setInteger(DialogResult::Ok);
}


// -- TextEntryDialog class implementation --

// (CLASS) TextEntryDialog
// Category: Dialog
// Description: A class that represents a text entry dialog.
// Remarks: Class that represents a text entry dialog.

TextEntryDialog::TextEntryDialog()
{
    m_style = wxOK | wxCANCEL | wxCENTRE;
    m_message = wxEmptyString;
    m_caption = _("Enter Text");
    m_text = wxEmptyString;
}

TextEntryDialog::~TextEntryDialog()
{
}

// (CONSTRUCTOR) TextEntryDialog.constructor
// Description: Creates a new TextEntryDialog.
//
// Syntax: TextEntryDialog()
//
// Remarks: Creates a new TextEntryDialog.

void TextEntryDialog::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

// (METHOD) TextEntryDialog.setText
// Description: Sets the text of the text entry dialog.
//
// Syntax: function TextEntryDialog.setText(text : String)
//
// Remarks: Sets the |text| of the text entry dialog.
//
// Param(text): The |text| to which to set the text of the dialog.

void TextEntryDialog::setText(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_text = towx(env->getParam(0)->getString());
}

// (METHOD) TextEntryDialog.getText
// Description: Gets the text of the text entry dialog.
//
// Syntax: function TextEntryDialog.getText() : String
//
// Remarks: Returns the text of the text entry dialog.
//
// Returns: Returns the text of the text entry dialog.

void TextEntryDialog::getText(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setString(towstr(m_text));
}

// (METHOD) TextEntryDialog.setMessage
// Description: Sets the message to display in the text entry dialog.
//
// Syntax: function TextEntryDialog.setMessage(text : String)
//
// Remarks: Sets the message to display in the text entry dialog to |text|.
//
// Param(text): Sets the message to display in the text entry dialog to |text|.

void TextEntryDialog::setMessage(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_message = towx(env->getParam(0)->getString());
}

// (METHOD) TextEntryDialog.getMessage
// Description: Gets the message to display in the text entry dialog.
//
// Syntax: function TextEntryDialog.getMessage() : String
//
// Remarks: Returns the message to display in the text entry dialog.
//
// Returns: Returns the message to display in the text entry dialog.

void TextEntryDialog::getMessage(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setString(towstr(m_message));
}

// (METHOD) TextEntryDialog.setCaption
// Description: Sets the caption for the text entry dialog.
//
// Syntax: function TextEntryDialog.setCaption(text : String)
//
// Remarks: Sets the caption of the text entry dialog to |text|.
//
// Param(text): The |text| to which to set the caption of the text entry dialog.

void TextEntryDialog::setCaption(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_caption = towx(env->getParam(0)->getString());
}

// (METHOD) TextEntryDialog.getCaption
// Description: Gets the caption of the text entry dialog.
//
// Syntax: function TextEntryDialog.getCaption() : String
//
// Remarks: Returns the caption of the text entry dialog.
//
// Returns: Returns the caption of the text entry dialog.

void TextEntryDialog::getCaption(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setString(towstr(m_caption));
}

// (METHOD) TextEntryDialog.showDialog
// Description: Shows the text entry dialog.
//
// Syntax: function TextEntryDialog.showDialog() : Boolean
//
// Remarks: Shows a text entry dialog and returns DialogResult.Ok if the user
//      closes the text entry dialog by pressing "OK", or the equivalent, and
//      DialogResult.Cancel if the user closes the text entry dialog by
//      pressing "Cancel", or the equivalent.
//
// Returns: Returns DialogResult.Ok when the dialog is closed by pressing
//      "OK" or equivalent, and DialogResult.Cancel if the dialog is closed
//      by pressing "Cancel" or equivalent.

void TextEntryDialog::showDialog(kscript::ExprEnv* env, kscript::Value* retval)
{
    wxTextEntryDialog dlg(g_app->getMainWindow(),
                          m_message,
                          m_caption,
                          m_text,
                          m_style);

    int res = dlg.ShowModal();
    if (res != wxID_OK)
    {
        m_caption = dlg.GetLabel();
        m_text = dlg.GetValue();
        retval->setInteger(DialogResult::Cancel);
        return;
    }

    m_caption = dlg.GetLabel();
    m_text = dlg.GetValue();
    retval->setInteger(DialogResult::Ok);
}




// -- PasswordEntryDialog class implementation --

// (CLASS) PasswordEntryDialog
// Category: Dialog
// Derives: TextEntryDialog
// Description: A class that represents a password entry dialog.
// Remarks: Class that represents a password entry dialog.

PasswordEntryDialog::PasswordEntryDialog()
{
    m_style = wxOK | wxCANCEL | wxCENTRE | wxTE_PASSWORD;
}

PasswordEntryDialog::~PasswordEntryDialog()
{
}

// (CONSTRUCTOR) PasswordEntryDialog.constructor
// Description: Creates a new PasswordEntryDialog.
//
// Syntax: PasswordEntryDialog()
//
// Remarks: Creates a new PasswordEntryDialog.

void PasswordEntryDialog::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    TextEntryDialog::constructor(env, retval);
    m_caption = _("Enter Password");
}


// -- ColorDialog class implementation --

// (CLASS) ColorDialog
// Category: Dialog
// Description: A class that represents a color chooser dialog.
// Remarks: Class that represents a color chooser dialog.

ColorDialog::ColorDialog()
{
    m_caption = _("Choose Color");
}

ColorDialog::~ColorDialog()
{
}

// (CONSTRUCTOR) ColorDialog.constructor
// Description: Creates a new ColorDialog.
//
// Syntax: ColorDialog()
//
// Remarks: Creates a new ColorDialog.

void ColorDialog::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

// (METHOD) ColorDialog.setCaption
// Description: Sets the caption for the color dialog.
//
// Syntax: function ColorDialog.setCaption(text : String)
//
// Remarks: Sets the caption of the color dialog to |text|.
//
// Param(text): The |text| to which to set the caption of the color dialog.

void ColorDialog::setCaption(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_caption = towx(env->getParam(0)->getString());
}

// (METHOD) ColorDialog.getCaption
// Description: Gets the caption of the color dialog.
//
// Syntax: function ColorDialog.getCaption() : String
//
// Remarks: Returns the caption of the color dialog.
//
// Returns: Returns the caption of the color dialog.

void ColorDialog::getCaption(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setString(towstr(m_caption));
}

// (METHOD) ColorDialog.setColor
// Description: Sets the color of the color dialog.
//
// Syntax: function ColorDialog.setColor(color : Color)
// Syntax: function ColorDialog.setColor(red : Integer,
//                                       green : Integer,
//                                       blue : Integer)
//
// Remarks: Sets the color of the color dialog to the
//     value specified by |color| or to the values specified
//     by |red|, |green|, and |blue|.
//
// Param(color): The |color| object to which to set the dialog's color.
// Param(red): The |red| value of the color to which to set the dialog's color.
// Param(green): The |green| value of the color to which to set the dialog's color.
// Param(blue): The |blue| value of the color to which to set the dialog's color.

void ColorDialog::setColor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // -- parameter 1 is a Color object --
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    if (obj->getClassName() == L"Color")
    {
        Color* c = (Color*)obj;
        
        wxColour c2(c->getMember(L"red")->getInteger(),
                    c->getMember(L"green")->getInteger(),
                    c->getMember(L"blue")->getInteger());
        m_color_data.SetColour(c2);
        return;
    }

    // -- parameters 1, 2 and 3 are RGB values --
    wxColour c2(env->getParam(0)->getInteger(),
                env->getParam(1)->getInteger(),
                env->getParam(2)->getInteger());
    m_color_data.SetColour(c2);
}

// (METHOD) ColorDialog.getColor
// Description: Gets the color of the color dialog.
//
// Syntax: function ColorDialog.getColor() : Color
//
// Remarks: Returns the color object that corresponds to the
//     currently set color of the color dialog.
//
// Returns: Returns the color object that corresponds to the
//     currently set color of the color dialog.

void ColorDialog::getColor(kscript::ExprEnv* env, kscript::Value* retval)
{
    wxColour c2 = m_color_data.GetColour();
    
    kscript::Value val;
    env->createObject(L"Color", &val);
    val.getMember(L"red")->setInteger(c2.Red());
    val.getMember(L"green")->setInteger(c2.Green());
    val.getMember(L"blue")->setInteger(c2.Blue());
    retval->setValue(val);
}

// (METHOD) ColorDialog.setCustomColor
// Description: Sets a custom color in the color dialog.
//
// Syntax: function ColorDialog.setCustomColor(index : Integer)
//                                             color : Color)
// Syntax: function ColorDialog.setCustomColor(index : Integer,
//                                             red : Integer,
//                                             green : Integer,
//                                             blue : Integer)
//
// Remarks: Sets the custom color with the given |index|, where
//     |index| is between 0 and 15, to the color specified by 
//     |color| or to the values specified by |red|, |green|, and 
//     |blue|.
//
// Param(index): The |index| of the custom color to set, where |index|
//     is between 0 and 15.
// Param(color): The |color| object to which to set the custom color.
// Param(red): The |red| value of the color to which to set the custom color.
// Param(green): The |green| value of the color to which to set the custom color.
// Param(blue): The |blue| value of the color to which to set the custom color.

void ColorDialog::setCustomColor(kscript::ExprEnv* env, kscript::Value* retval)
{
    int idx = env->getParam(0)->getInteger();
    
    // -- index is out of bounds --
    if (idx < 0 || idx > 15)
        return;
        
    // -- parameter 2 is a Color object --
    kscript::ValueObject* obj = env->getParam(1)->getObject();
    if (obj->getClassName() == L"Color")
    {
        Color* c = (Color*)obj;
        
        wxColour c2(c->getMember(L"red")->getInteger(),
                    c->getMember(L"green")->getInteger(),
                    c->getMember(L"blue")->getInteger());
        m_color_data.SetCustomColour(idx, c2);
        return;
    }

    // -- parameters 2, 3 and 4 are RGB values --
    wxColour c2(env->getParam(1)->getInteger(),
                env->getParam(2)->getInteger(),
                env->getParam(3)->getInteger());
    m_color_data.SetCustomColour(idx, c2);
}

// (METHOD) ColorDialog.getCustomColor
// Description: Gets a custom color from the color dialog.
//
// Syntax: function ColorDialog.getCustomColor(index : Integer) : Color
//
// Remarks: Returns the custom color object for the specified
//     |index|, where |index| is between 0 and 15.
//
// Param(index): The |index| of the custom color to return.
//
// Returns: Returns the custom color object for the specified
//     |index|, where |index| is between 0 and 15.

void ColorDialog::getCustomColor(kscript::ExprEnv* env, kscript::Value* retval)
{
    int idx = env->getParam(0)->getInteger();
    wxColour c2 = m_color_data.GetCustomColour(idx);
    
    kscript::Value val;
    env->createObject(L"Color", &val);
    val.getMember(L"red")->setInteger(c2.Red());
    val.getMember(L"green")->setInteger(c2.Green());
    val.getMember(L"blue")->setInteger(c2.Blue());
    retval->setValue(val);
}

// (METHOD) ColorDialog.showAllColors
// Description: Sets a flag to show either a full color dialog or a 
//     basic color dialog.
//
// Syntax: function ColorDialog.showAllColors(flag : Boolean)
//
// Remarks: Sets a |flag| to show either a full color dialog or a 
//     basic color dialog.  If |flag| is true, a full color dialog will
//     be shown. If |flag| is false, a basic color dialog will be shown.
//
// Param(flag): A |flag| to determine whether to show a full color dialog,
//     or a basic color dialog.

void ColorDialog::showAllColors(kscript::ExprEnv* env, kscript::Value* retval)
{
    bool show = true;
    if (env->getParamCount() > 0 && env->getParam(0)->getBoolean() == false)
        show = false;
    
    m_color_data.SetChooseFull(show);
}

// (METHOD) ColorDialog.showDialog
// Description: Shows the color dialog.
//
// Syntax: function ColorDialog.showDialog() : Boolean
//
// Remarks: Shows a color dialog and returns DialogResult.Ok if the user
//      closes the color dialog by pressing "OK", or the equivalent, and
//      DialogResult.Cancel if the user closes the color dialog by pressing
//      "Cancel", or the equivalent.
//
// Returns: Returns DialogResult.Ok when the dialog is closed by pressing
//      "OK" or equivalent, and DialogResult.Cancel if the dialog is closed
//      by pressing "Cancel" or equivalent.

void ColorDialog::showDialog(kscript::ExprEnv* env, kscript::Value* retval)
{
    wxColourDialog dlg(g_app->getMainWindow(),
                       &m_color_data);
    dlg.SetTitle(m_caption);
    
    int res = dlg.ShowModal();
    if (res != wxID_OK)
    {
        retval->setInteger(DialogResult::Cancel);
        return;
    }

    m_color_data = dlg.GetColourData();
    retval->setInteger(DialogResult::Ok);
}


// -- ProjectFileDialog class implementation --

// (CLASS) ProjectFileDialog
// Category: Dialog
// Description: A class that represents a project file dialog.
// Remarks: Class that represents a project file dialog.
//
// Property(ProjectFileDialog.OpenFile):        A flag representing the open project file dialog.
// Property(ProjectFileDialog.SaveFile):        A flag representing the save project file dialog.
// Property(ProjectFileDialog.ChooseFolder):    A flag representing the choose project folder dialog.

ProjectFileDialog::ProjectFileDialog()
{
    m_mode = ProjectFileDialog::OpenFile;
    m_path = wxEmptyString;
    m_caption = wxEmptyString;
    m_caption_set = false;
}

ProjectFileDialog::~ProjectFileDialog()
{

}

// (CONSTRUCTOR) ProjectFileDialog.constructor
// Description: Creates a new ProjectFileDialog.
//
// Syntax: ProjectFileDialog(mode : Integer)
//
// Remarks: Creates a new ProjectFileDialog based on the
//     specified |mode|. If |mode| is ProjectFileDialog.OpenFile,
//     then the dialog will open in file open mode. If |mode| is
//     ProjectFileDialog.SaveFile, then the dialog will open in
//     file save mode.  If |mode| is ProjectFileDialog.ChooseFolder,
//     then the dialog will open in folder choice mode.
//
// Param(mode): The |mode| specifies what type of ProjectFileDialog
//     will be created and is one of ProjectFileDialog.OpenFile,
//     ProjectFileDialog.SaveFile, or ProjectFileDialog.ChooseFolder

void ProjectFileDialog::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    int mode = env->getParam(0)->getInteger();
    if (mode == ProjectFileDialog::OpenFile ||
        mode == ProjectFileDialog::SaveFile ||
        mode == ProjectFileDialog::ChooseFolder)
    {
        m_mode = mode;
    }
}

// (METHOD) ProjectFileDialog.setPath
// Description: Sets the initial path to show in the project file dialog.
//
// Syntax: function ProjectFileDialog.setPath(text : String)
//
// Remarks: Sets the initial path to show in the project file dialog.
//
// Param(text): The initial path to show in the project file dialog.

void ProjectFileDialog::setPath(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_path = towx(env->getParam(0)->getString());
}

// (METHOD) ProjectFileDialog.getPath
// Description: Gets the path selected by the user.
//
// Syntax: function ProjectFileDialog.getPath() : String
//
// Remarks: Returns the path selected by the user.
//
// Returns: Returns the path selected by the user.

void ProjectFileDialog::getPath(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setString(towstr(m_path));
}

// (METHOD) ProjectFileDialog.setCaption
// Description: Sets the caption for the project file dialog.
//
// Syntax: function ProjectFileDialog.setCaption(text : String)
//
// Remarks: Sets the caption of the project file dialog to |text|.
//
// Param(text): The |text| to which to set the caption of the project file dialog.

void ProjectFileDialog::setCaption(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_caption = towx(env->getParam(0)->getString());
    m_caption_set = true;
}

// (METHOD) ProjectFileDialog.getCaption
// Description: Gets the caption of the project file dialog.
//
// Syntax: function ProjectFileDialog.getCaption() : String
//
// Remarks: Returns the caption of the project file dialog.
//
// Returns: Returns the caption of the project file dialog.

void ProjectFileDialog::getCaption(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setString(towstr(m_caption));
}

// (METHOD) ProjectFileDialog.showDialog
// Description: Shows the project file dialog.
//
// Syntax: function ProjectFileDialog.showDialog() : Boolean
//
// Remarks: Shows a project file dialog and returns DialogResult.Ok if
//      the user closes the project file dialog by pressing "OK", or the
//      equivalent, and DialogResult.Cancel if the user closes the project
//      file dialog by pressing "Cancel", or the equivalent.
//
// Returns: Returns DialogResult.Ok when the dialog is closed by pressing
//      "OK" or equivalent, and DialogResult.Cancel if the dialog is closed
//      by pressing "Cancel" or equivalent.

void ProjectFileDialog::showDialog(kscript::ExprEnv* env, kscript::Value* retval)
{
    int dlg_mode;
    switch (m_mode)
    {
        default:
        case OpenFile:
            dlg_mode = DlgDatabaseFile::modeOpen;
            break;
        case SaveFile:
            dlg_mode = DlgDatabaseFile::modeSave;
            break;
        case ChooseFolder:
            dlg_mode = DlgDatabaseFile::modeSelectFolder;
            break;
    }
    
    DlgDatabaseFile dlg(g_app->getMainWindow(), dlg_mode);
    
    // since there are three modes to this dialog, we only want to call
    // setCaption() here if the caption has explicitly been set
    if (m_caption_set)
        dlg.setCaption(m_caption);
    
    dlg.setPath(m_path);
    
    if (dlg_mode == DlgDatabaseFile::modeSave)
        dlg.setOverwritePrompt(true);

    int res = dlg.ShowModal();

    if (res != wxID_OK)
    {
        m_caption = dlg.getCaption();
        m_path = dlg.getPath();
        retval->setInteger(DialogResult::Cancel);
        return;
    }

    m_caption = dlg.getCaption();
    m_path = dlg.getPath();
    retval->setInteger(DialogResult::Ok);
}




