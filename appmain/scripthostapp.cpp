/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-01-28
 *
 */


#include "appmain.h"
#include "appcontroller.h"
#include "scripthost.h"
#include "scripthostapp.h"
#include "scriptgui.h"
#include "scriptmenu.h"
#include "scriptdb.h"
#include "scriptbitmap.h"
#include "scriptwebdom.h"
#include "jobexport.h"
#include "jobexportpkg.h"
#include "jobimport.h"
#include "jobimportpkg.h"
#include "jobcopy.h"
#include "extensionpkg.h"
#include "reportlayout.h"
#include "tabledoc.h"
#include "querydoc.h"
#include "webdoc.h"
#include "editordoc.h"
#include "reportdoc.h"
#include "panelconsole.h"
#include <kl/url.h>
#include <kl/thread.h>
#include <../kscript/json.h>



// (CLASS) Console
// Category: HostApp
// Description: A class that provides a mechanism to control what is displayed
//     in the application's output console.
// Remarks: The Console class  provides a mechanism to control what is displayed
//     in the application's output console.  Text can be appended, the console can
//     be shown or hidden, and the text inside the console can be cleared.

Console::Console()
{
}

Console::~Console()
{
}

void Console::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

// (METHOD) Console.show
// Description: Shows or hides the form control.
//
// Syntax: function Console.show(flag : Boolean)
//
// Remarks: Shows the console if the |flag| is true.  Hides the
//     console if the |flag| is false.  If |flag| isn't specified,
//     the console is shown.
//
// Param(flag): A |flag| which indicates whether to show or hide
//     the console.

void Console::show(kscript::ExprEnv* env, void*, kscript::Value* retval)
{
    bool show = true;
    if (env->getParamCount() > 0)
        show = env->getParam(0)->getBoolean();
    g_app->getAppController()->showConsolePanel(show);
}

// (METHOD) Console.write
// Description: Outputs text to the console
//
// Syntax: function Console.write(text : String)
//
// Remarks: Outputs the string passed to the |text| parameter to
//     the console.  A new-line is not automatically added.
//
// Param(text): The string to append to the console output

void Console::write(kscript::ExprEnv* env, void*, kscript::Value* retval)
{
    wxString message = towx(env->m_eval_params[0]->getString());
    IFramePtr frame = g_app->getMainFrame();
    if (frame.isOk())
    {
        IDocumentSitePtr site = frame->lookupSite(wxT("ConsolePanel"));
        if (site.isOk())
        {
            IConsolePanelPtr console = site->getDocument();
            if (console.isOk())
                console->print(message);
        }
    }
}

// (METHOD) Console.writeLine
// Description: Outputs text to the console
//
// Syntax: function Console.writeLine(text : String)
//
// Remarks: Outputs the string passed to the |text| parameter to
//     the console.  A new-line is automatically added to the string.
//
// Param(text): The string to append to the console output

void Console::writeLine(kscript::ExprEnv* env, void*, kscript::Value* retval)
{
    wxString message = towx(env->m_eval_params[0]->getString());
    message += wxT("\n");
    
    IFramePtr frame = g_app->getMainFrame();
    if (frame.isOk())
    {
        IDocumentSitePtr site = frame->lookupSite(wxT("ConsolePanel"));
        if (site.isOk())
        {
            IConsolePanelPtr console = site->getDocument();
            if (console.isOk())
                console->print(message);
        }
    }
}

// (METHOD) Console.clear
// Description: Clears the console output
//
// Syntax: function Console.clear()
//
// Remarks: Clears all text in the output console

void Console::clear(kscript::ExprEnv* env, void*, kscript::Value* retval)
{
    IFramePtr frame = g_app->getMainFrame();
    if (frame.isOk())
    {
        IDocumentSitePtr site = frame->lookupSite(wxT("ConsolePanel"));
        if (site.isOk())
        {
            IConsolePanelPtr console = site->getDocument();
            if (console.isOk())
                console->clear();
        }
    }
}


// (CLASS) Extension
// Category: HostApp
// Description: A class that provides information about and access to extensions.
// Remarks: The Extension class provides information about and access to
//     extension packages.  Many times bitmap or text resources need to
//     be loaded from the extension package to be displayed in the user
//     interface of the extension.  This class facilitates this functionality.

Extension::Extension()
{
}

Extension::~Extension()
{
}

void Extension::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

// (METHOD) Extension.getBitmapResource
//
// Description: Loads a bitmap resource from an extension
//
// Syntax: static function Extension.getBitmapResource(path : String) : Bitmap
//
// Remarks: Loads a bitmap resource from an extension package.  If the
//     extension is packaged up into a kxt/zip file, the resource is loaded
//     from that archive.  If the extension is not packaged up, but is rather
//     one or more non-packaged files in a directory, the resource is loaded
//     from that directory.
//
// Param(path): The path to the desired resource either in the extension
//     file or extension directory
// Returns: A valid bitmap object upon success, null otherwise

void Extension::getBitmapResource(kscript::ExprEnv* env, void*, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
    {
        // invalid number of parameters
        retval->setNull();
        return;
    }
    
    std::wstring filename = env->getParam(0)->getString();
    int format = Bitmap::FormatDefault;
    wxBitmapType bmp_type;
    
    if (env->getParamCount() > 1)
    {
        format = env->getParam(1)->getInteger();
    }
    
    switch (format)
    {
        case Bitmap::FormatDefault: bmp_type = wxBITMAP_TYPE_ANY; break;
        case Bitmap::FormatBMP: bmp_type = wxBITMAP_TYPE_BMP; break;
        case Bitmap::FormatXPM: bmp_type = wxBITMAP_TYPE_XPM; break;
        case Bitmap::FormatGIF: bmp_type = wxBITMAP_TYPE_GIF; break;
        case Bitmap::FormatPNG: bmp_type = wxBITMAP_TYPE_PNG; break;
        case Bitmap::FormatJPG: bmp_type = wxBITMAP_TYPE_JPEG; break;
        default:
            // unrecognized bitmap format
            retval->setBoolean(false);
            return;
    }

    // try to get package reference
    ExtensionPkg* pkg = NULL;
    ScriptHost* script_host = (ScriptHost*)env->getParser()->getExtraLong();
    if (script_host)
        pkg = script_host->getPackage();

    wxBitmap bmp;
    
    if (pkg)
    {
        // load the image from the extension package
        
        wxInputStream* stream = pkg->getStream(towx(filename));
        if (!stream)
        {
            // resource doesn't exist, return failure
            retval->setNull();
            return;
        }
        
        wxImage image;
        if (!image.LoadFile(*stream, bmp_type))
        {
            // resource couldn't be loaded, perhaps because
            // of a bad/mismatched format
            retval->setNull();
            return;
        }
        
        bmp = wxBitmap(image);
    }
     else
    {
        // load the image from the directory the script is in
        
        wxString path = script_host->getStartupPath();
        if (path.Length() == 0 || path.Last() != PATH_SEPARATOR_CHAR)
            path += PATH_SEPARATOR_CHAR;
            
        if (filename.length() > 0 && filename[0] == PATH_SEPARATOR_CHAR)
            path.RemoveLast();
        path += towx(filename);
        
        if (!xf_get_file_exist(towstr(path)))
        {
            // file doesn't exist
            retval->setNull();
            return;
        }
        
        if (!bmp.LoadFile(path, bmp_type))
        {
            // file couldn't be loaded / format mismatch
            retval->setNull();
            return;
        }
    }
    
    
    // load the bitmap
    
    Bitmap* bitmap_obj = Bitmap::createObject(env);
    bitmap_obj->setWxBitmap(bmp);
    retval->setObject(bitmap_obj);
}

// (METHOD) Extension.getTextResource
//
// Description: Loads a text resource from an extension
//
// Syntax: static function Extension.getTextResource(path : String) : String
//
// Remarks: Loads a text resource from an extension package.  If the
//     extension is packaged up into a kxt/zip file, the resource is loaded
//     from that archive.  If the extension is not packaged up, but is rather
//     one or more non-packaged files in a directory, the resource is loaded
//     from that directory.
//
// Param(path): The path to the desired resource either in the extension
//     file or extension directory
// Returns: A valid string upon success, null otherwise

void Extension::getTextResource(kscript::ExprEnv* env, void*, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
    {
        // invalid number of parameters
        retval->setNull();
        return;
    }
    
    std::wstring filename = env->getParam(0)->getString();
    std::string text;
    

    // try to get package reference
    ExtensionPkg* pkg = NULL;
    ScriptHost* script_host = (ScriptHost*)env->getParser()->getExtraLong();
    if (script_host)
        pkg = script_host->getPackage();

   
    if (pkg)
    {
        // load the image from the extension package
        
        wxInputStream* stream = pkg->getStream(towx(filename));
        if (!stream)
        {
            // resource doesn't exist, return failure
            retval->setNull();
            return;
        }

            
        unsigned char* buf = new unsigned char[16400];
        while (1)
        {
            stream->Read(buf, 16384);
            size_t read = stream->LastRead();
            if (read == 0)
                break;
                
            buf[read] = 0;
            text += (char*)buf;
        }
        delete[] buf;
    }
     else
    {
        // load the image from the directory the script is in
        
        wxString path = script_host->getStartupPath();
        if (path.Length() == 0 || path.Last() != PATH_SEPARATOR_CHAR)
            path += PATH_SEPARATOR_CHAR;
            
        if (filename.length() > 0 && filename[0] == PATH_SEPARATOR_CHAR)
            path.RemoveLast();
        path += towx(filename);
        
        if (!xf_get_file_exist(towstr(path)))
        {
            // file doesn't exist
            retval->setNull();
            return;
        }
        

        xf_file_t f = xf_open(towstr(path), xfOpen, xfRead, xfShareRead);
        if (!f)
        {
            // cannot open file
            retval->setNull();
            return;
        }
        
        unsigned char* buf = new unsigned char[16400];
        while (1)
        {
            int read = xf_read(f, buf, 1, 16384);
            if (read == 0)
                break;
            buf[read] = 0;
            text += (char*)buf;
            if (read != 16384)
                break;
        }
        delete[] buf;
    
        xf_close(f);
    }

    std::wstring res = kl::towstring(text);
    retval->setString(res);
}

// (METHOD) Extension.isContextPackage
//
// Description: Determines if the program is running from a package
//
// Syntax: static function Extension.isContextPackage() : Boolean
//
// Remarks: Calling this method allows the caller to determine if the
//     program running is running in from a context of an extension package.
//
// Returns: True if the program is running from an extension package, false otherwise

void Extension::isContextPackage(kscript::ExprEnv* env, void*, kscript::Value* retval)
{
    // try to get package reference
    ExtensionPkg* pkg = NULL;
    ScriptHost* script_host = (ScriptHost*)env->getParser()->getExtraLong();
    if (script_host)
        pkg = script_host->getPackage();

    retval->setBoolean(pkg ? true : false);
}


// (CLASS) HostApp
// Category: HostApp
// Description: A class that provides a mechanism of communicating with the 
//     script's host application.
// Remarks: The HostApp class is an important class for interfacing with the
//     host application.  It provides access to the host application's frame,
//     menus, document infrastructure, job queue, and other important aspects.

// These are undocumented as-of-yet and should remain
// as such until we expose the createPane() method
// Property(HostApp.DockLeft):      A flag representing that a pane should be docked at the left of the client area.
// Property(HostApp.DockTop):       A flag representing that a pane should be docked at the top of the client area.
// Property(HostApp.DockBottom):    A flag representing that a pane should be docked at the bottom of the client area.
// Property(HostApp.DockRight):     A flag representing that a pane should be docked at the right of the client area.
// Property(HostApp.DockFloating):  A flag representing that a pane should float above the client area.

// (EVENT) HostApp.commandEntered
// Description: Fired when a command is entered in the console.
// Param(value): The text of the command that was entered in the console.

// (EVENT) HostApp.activeChildChanged
// Description: Fired when the active child document changes.
// Param(location): The location of the new child document.
// Param(caption): The caption of the new child document.

// (EVENT) HostApp.locationChanged
// Description: Fired when the location in the URL bar changes.
// Param(location): The new location.
// Param(caption): The caption of the document at the new location.

// (EVENT) HostApp.frameDestroyed
// Description: Fired when the main application frame is destroyed.


HostApp::HostApp()
{
}

HostApp::~HostApp()
{
}


void HostApp::init(kscript::ExprParser* expr, Application* app)
{
    m_app.setApp(app);
    
    m_frame = g_app->getMainFrame();
    
    if (m_frame)
    {
        // connect the signal handlers, if there is a frame window (gui mode)
        // console mode doesn't have a frame (m_frame == NULL)
        m_frame->sigActiveChildChanged().connect(this, &HostApp::onActiveChildChanged);
        m_frame->sigFrameEvent().connect(this, &HostApp::onFrameEvent);
        m_frame->sigFrameDestroy().connect(this, &HostApp::onFrameDestroy);
    }
    
    // -- add the events --
    getMember(L"commandEntered")->setObject(Event::createObject(expr));
    getMember(L"activeChildChanged")->setObject(Event::createObject(expr));
    getMember(L"locationChanged")->setObject(Event::createObject(expr));
    getMember(L"frameDestroyed")->setObject(Event::createObject(expr));
    
    getMember(L"DockTop")->setInteger(HostApp::DockTop);
    getMember(L"DockLeft")->setInteger(HostApp::DockLeft);
    getMember(L"DockBottom")->setInteger(HostApp::DockBottom);
    getMember(L"DockRight")->setInteger(HostApp::DockRight);
    getMember(L"DockFloating")->setInteger(HostApp::DockFloating);
    
    getMember(L"ExecuteWait")->setInteger(HostApp::ExecuteWait);
    getMember(L"ExecuteSource")->setInteger(HostApp::ExecuteSource);
    getMember(L"ExecuteTemplate")->setInteger(HostApp::ExecuteTemplate);
}

void HostApp::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}



// (METHOD) HostApp.getDatabase
//
// Description: Returns the application's current database connection
//
// Syntax: function HostApp.getDatabase() : DbConnection
//
// Remarks: Returns a DbConnection object representing the application's
//     current database connection.  Using this object allows programs
//     to access and manipulate the host application's current database
//     project.
//
// Returns: A DbConnection object


void HostApp::getDatabase(kscript::ExprEnv* env, kscript::Value* retval)
{
    DbConnection* db = DbConnection::createObject(env);
    db->setDatabase(g_app->getDatabase());
    retval->setObject(db);
}


// (METHOD) HostApp.createDatabase
//
// Description: Creates a new database
//
// Syntax: function HostApp.createDatabase(location : String, type : DbDatabaseType) : Boolean
//
// Remarks: Creates a new database at the specified location.  The type of
//     database created is indicated by the |type| parameter, which is one
//     of the values included in the DbDatabaseType enumeration.
//
// Returns: |true| if the operation succeeded, otherwise |false| if an error occurred


void HostApp::createDatabase(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
    
    tango::IDatabaseMgrPtr dbmgr = tango::getDatabaseMgr();
    if (dbmgr.isNull())
        return;
    
    if (env->getParamCount() < 2)
        return;
        
    int db_type = DbDatabaseType::toTangoDatabaseType(env->getParam(1)->getInteger());
    
    bool result = dbmgr->createDatabase(env->getParam(0)->getString(), db_type);
    
    retval->setBoolean(result);
}


// (METHOD) HostApp.openDatabase
//
// Description: Opens a new database project in the host application
//
// Syntax: function HostApp.openDatabase(connection : String) : Boolean
// Syntax: function HostApp.openDatabase(database : DbConnection) : Boolean
//
// Remarks: Opens a database as the host application's current database project.
//     The database may be specified with a connection string, a path to the
//     database, or a database connection object.
//
// Returns: |true| if the operation succeeded, otherwise |false| if an error occurred

void HostApp::openDatabase(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
    
    if (env->getParamCount() < 1)
        return;
    
    if (env->getParam(0)->isString())
    {
        std::wstring cstr = env->getParam(0)->getString();
        if (cstr == L"")
        {
            g_app->getAppController()->closeProject();
            return;
        }
        
        tango::IDatabaseMgrPtr dbmgr = tango::getDatabaseMgr();
        if (dbmgr.isNull())
            return;
        
        tango::IDatabasePtr db = dbmgr->open(cstr);
        if (db.isNull())
            return;
            
        bool result = g_app->getAppController()->openProject(db);
        retval->setBoolean(result);
    }
     else if (env->getParam(0)->isObject())
    {
        kscript::ValueObject* vobj = env->getParam(0)->getObject();
        if (vobj->isKindOf(L"DbConnection"))
        {
            DbConnection* conn = (DbConnection*)vobj;
            tango::IDatabasePtr db = conn->getDatabase();
            if (db.isOk())
            {
                bool result = g_app->getAppController()->openProject(db);
                retval->setBoolean(result);
            }
        }
    }
}



// (METHOD) HostApp.getCurrentLocation
//
// Description: Returns the current location in the URL bar.
//
// Syntax: function HostApp.getCurrentLocation() : String
//
// Remarks: Returns the current location in the URL bar.
//
// Returns: Returns a string containing the current location in the URL bar.

void HostApp::getCurrentLocation(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_frame.isNull())
    {
        retval->setString(towstr(wxT("")));
        return;
    }

    // return the location of the active document
    IDocumentSitePtr doc_site = m_frame->getActiveChild();
    if (doc_site.isOk())
    {
        IDocumentPtr doc = doc_site->getDocument();
        if (doc.isOk())
        {
            wxString location, caption;
            location = doc->getDocumentLocation();
            retval->setString(location);
            return;
        }
    }
    
    // if we don't have an active document, return a 
    // blank string
    retval->setString(towstr(wxT("")));
    return;
}

// (METHOD) HostApp.setFrameCaption
//
// Description: Sets the frame's caption
//
// Syntax: function HostApp.setFrameCaption(caption : String)   
//
// Remarks: By default, the host application sets its own frame caption.
//     A call to HostApp.setFrameCaption() allows the script application
//     to modify the frame caption of the host application.
//
// Param(caption): A string containing the new frame caption

void HostApp::setFrameCaption(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() <= 0)
        return;

    if (m_frame.isNull())
        return;

    m_frame->getFrameWindow()->SetTitle(towx(env->getParam(0)->getString()));
}


// (METHOD) HostApp.getFrameCaption
//
// Description: Gets the frame's caption
//
// Syntax: function HostApp.getFrameCaption() : String
//
// Remarks: A call to HostApp.getFrameCaption() allows the script application
//     to get the existing frame caption of the host application.
//
// Returns: Returns a string containing the frame caption

void HostApp::getFrameCaption(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_frame.isNull())
    {
        retval->setString(towstr(wxT("")));
        return;
    }

    wxString caption = m_frame->getFrameWindow()->GetTitle();
    retval->setString(towstr(caption));
}


// (METHOD) HostApp.getFrameMenu
//
// Description: Returns an object representing the frame's menu bar 
//
// Syntax: function HostApp.getFrameMenu() : MenuBar       
//
// Remarks: A call to getFrameMenu() returns a MenuBar object which represents
//     the host application's menu bar.  This is useful for script applications
//     that wish to add or remove menu items from the host application's
//     menu hierarchy.
//
// Returns: A MenuBar object. Null is returned if there is no menu.

void HostApp::getFrameMenu(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_frame.isNull())
    {
        retval->setNull();
        return;
    }
    
    wxMenuBar* menu_bar = m_frame->getFrameWindow()->GetMenuBar();
    if (!menu_bar)
    {
        // since there is no menu bar, return null
        retval->setNull();
        return;
    }
    
    MenuBar* menu = MenuBar::createObject(env);
    menu->setWxMenuBar(menu_bar);
    retval->setObject(menu);
}


// (METHOD) HostApp.showFrame
//
// Description: Shows or hides the host application's main window
//
// Syntax: function HostApp.showFrame(show : Boolean)
//
// Remarks: Shows the host application's main window if the |show|
//     parameter is true, and hides it if the parameter is false.
//
// Param(show): A boolean value of true or false, directing the host
//     application's main window to either show or hide.  If this 
//     parameter is omitted, a value of true is assumed.

void HostApp::showFrame(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_frame.isNull())
        return;

    bool visible = true;
    if (env->getParamCount() > 0)
        visible = env->getParam(0)->getBoolean();
    
    m_frame->show(visible);
}


// (METHOD) HostApp.showJobManager
//
// Description: Shows the host application's job manager
//
// Syntax: function HostApp.showJobManager(show : Boolean)
//
// Remarks: Shows the host application's job manager if the |show|
//     parameter is true, and hides it if the parameter is false.
//
// Param(show): A boolean containing true or false, directing the window
//     to either show or hide.  If this parameter is omitted, a value of
//     true is assumed.


void HostApp::showJobManager(kscript::ExprEnv* env, kscript::Value* retval)
{
    bool show = true;
    
    if (env->getParamCount() > 0)
    {
        show = env->getParam(0)->getBoolean();
    }
    
    g_app->getAppController()->showJobManager(show);
}


// DO NOT DOCUMENT - deprecated
// HostApp.createJob() - creates a job object which can display in the
//                       host application's job queue
//
//   Return Value: HostJob object

void HostApp::createJob(kscript::ExprEnv* env, kscript::Value* retval)
{
    HostJob* job = HostJob::createObject(env);
    retval->setObject(job);
}



// (METHOD) HostApp.newDocument
//
// Description: Creates a new document in the host application
//
// Syntax: function HostApp.newDocument(doc_type : String) : HostDocument
//
// Remarks: Creates a new document in the host application.  The
//     document type can specify any document type that the
//     application supports.  Possible values are "table", "web",
//     "query", "editor", or "report"
//
// Returns: A valid HostDocument object upon success, |null|
//     in the case of an error.

void HostApp::newDocument(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();
    
    IFramePtr frame = g_app->getMainFrame();
    if (frame.isNull())
        return;
    
    std::wstring type = env->getParam(0)->getString();
    int site_id = 0;
    bool result = false;
    
    if (type == L"table")
        result = g_app->getAppController()->newTable(&site_id);
    else if (type == L"web")
        result = g_app->getAppController()->openWeb(L"about:blank", 0, appOpenDefault, &site_id);
    else if (type == L"query")
        result = g_app->getAppController()->newQuery(&site_id);
    else if (type == L"editor")
        result = g_app->getAppController()->newScript(&site_id);
    else if (type == L"report")
        result = g_app->getAppController()->newReport(&site_id);
        
    if (result && site_id != 0)
    {
        HostDocument* doc = HostDocument::createObject(env);
        doc->m_site = frame->lookupSiteById(site_id);
        retval->setObject(doc);
    }
}


// (METHOD) HostApp.open
//
// Description: Opens a database table, document, or web page
//
// Syntax: function HostApp.open(path : String, 
//                               mode : String) : HostDocument
//
// Remarks: Opens and browses a database table, document, or web page.
//     For some data types, such as text files, if the file is already
//     open, that document is instead shown.  A host document object
//     is returned.  If the document could not be opened, |null| is
//     returned
//
// Param(path): The location of the document to open
// Param(mode):  Optional.  Specifies how to open the document using
//     either "layout" or "design".  If unspecified, the document is
//     opened in the default view for that document.  Note: "mode" 
//     currently only applies to reports.
// Returns: A valid HostDocument object upon success, |null|
//     in the case of an error.

void HostApp::open(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();
    
    if (env->getParamCount() < 1)
        return;
    
    IFramePtr frame = g_app->getMainFrame();
    if (frame.isNull())
        return;

    int site_id = 0;
    std::wstring mode_string;
    
    std::wstring path = env->getParam(0)->getString();
    if (env->getParamCount() > 1)
        mode_string = env->getParam(1)->getString();

    int mode = appOpenDefault;
    if (mode_string == L"layout")
        mode |= appOpenInLayout;
  
    bool result = g_app->getAppController()->openAny(path, mode, &site_id);

    if (result && site_id != 0)
    {
        HostDocument* doc = HostDocument::createObject(env);
        doc->m_site = frame->lookupSiteById(site_id);
        retval->setObject(doc);
    }
}


// (METHOD) HostApp.openWeb
//
// Description: Opens a web document.
//
// Syntax: function HostApp.openWeb(url : String, 
//                                  post_params : Array)
//
// Remarks: Much of the functionality of openWeb() is similar to open().  The
//     openWeb() function, however, exposes additional functionality allowing
//     post parameters to be specified.  This is useful for filling out forms
//     and displaying the resulting web page in a browser
//
// Param(url): The url location of the page to open
// Param(post_params):  Optional.  If specified, the array should be a
//     key/value array where the key is the post variable name and the
//     value is the value for the post variable.

void HostApp::openWeb(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
        return;

    kscript::ValueObject* post_obj = NULL;
    wxWebPostData* post_data = NULL;
    
    if (env->getParamCount() >= 2)
    {
        kscript::Value* post_val = env->getParam(1);
        if (post_val->isObject())
            post_obj = post_val->getObject();
    }
        
    if (post_obj)
    {
        post_data = new wxWebPostData;
        
        size_t i, cnt = post_obj->getRawMemberCount();
        for (i = 0; i < cnt; ++i)
        {
            std::wstring member_name = post_obj->getRawMemberName(i);
            kscript::Value* member_obj = post_obj->getRawMemberByIdx(i);
            post_data->Add(towx(member_name), towx(member_obj->getString()));
        }
    }


    g_app->getAppController()->openWeb(towx(env->getParam(0)->getString()), post_data, appOpenForceNewWindow);
}


// (METHOD) HostApp.sendWebRequest
//
// Description: Sends a web request
//
// Syntax: function HostApp.sendWebRequest(url : String, 
//                                         post_params : Array) : String
//
// Remarks: The sendWebRequest() method is similar to the openWeb() method,
//     with one difference:  Instead of the resulting page being opened in
//     a browser window, it is returned to the application as a string.
//
// Param(url): The url location of the page to open
// Param(post_params):  Optional.  If specified, the array should be a
//     key/value array where the key is the post variable name and the
//     value is the value for the post variable.
// Returns: A string value containing the result of the web request.  A null value
//     indicates failure. An empty string likely indicates failure as well, though
//     certain web requests may normally result in an empty string.

void HostApp::sendWebRequest(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
        return;

    kscript::ValueObject* post_obj = NULL;
    wxWebPostData* post_data = NULL;
    
    if (env->getParamCount() >= 2)
    {
        kscript::Value* post_val = env->getParam(1);
        if (post_val->isObject())
            post_obj = post_val->getObject();
    }
        
    if (post_obj)
    {
        post_data = new wxWebPostData;
        
        size_t i, cnt = post_obj->getRawMemberCount();
        for (i = 0; i < cnt; ++i)
        {
            std::wstring member_name = post_obj->getRawMemberName(i);
            kscript::Value* member_obj = post_obj->getRawMemberByIdx(i);
            post_data->Add(towx(member_name), towx(member_obj->getString()));
        }
    }


    wxString result = wxEmptyString;
    
    wxWebControl::SaveRequestToString(towx(env->getParam(0)->getString()),
                              &result,
                              post_data);
    
    retval->setString(towstr(result));
}


// (METHOD) HostApp.close
// Description: Closes a specified document window
//
// Syntax: function HostApp.close(doc : HostDocument, 
//                                force : Boolean) : Boolean
//
// Remarks: Closes the document window represented by the specified
//     |doc| parameter.  If the window contains unsaved information,
//     the user will normally be prompted as to whether the information
//     should be saved.  If this prompting is not desired, passing true
//     in the optional |force| parameter will suppress this behavior.
//
// Param(doc):  The document to close.  This object may be obtained from
//     either the getActiveDocument() method or the getDocuments() method.
// Param(force):  Optional.  If true, suppresses any dialog prompting while
//     closing the document.  The default is false.
// Returns: Returns true upon success, false otherwise.

void HostApp::close(kscript::ExprEnv* env, kscript::Value* retval)
{
    bool force_close = false;
    
    if (env->getParamCount() < 1)
    {
        retval->setBoolean(false);
        return;
    }
     
    if (!env->getParam(0)->isObject() ||
        !env->getParam(0)->getObject()->isKindOf(L"HostDocument"))
    {
        retval->setBoolean(false);
        return;
    }
    
    if (env->getParamCount() >= 2)
    {
        force_close = env->getParam(1)->getBoolean();
    }
    
    
    HostDocument* doc = (HostDocument*)env->getParam(0)->getObject();
    bool success = m_frame->closeSite(doc->m_site, force_close ? closeForce : 0);
    
    retval->setBoolean(success);
}

// (METHOD) HostApp.getDocuments
// Description: Returns an array of open documents in the program.
//
// Syntax: function HostApp.getDocuments() : Array(HostDocument)
//
// Remarks: Returns an array of HostDocument objects which represent
//     all open documents inside the program.  A 'document' is a window
//     or container which holds information in the system, such as a web
//     control, table, report, or text editor.  See HostDocument for more
//     information on how documents can be manipulated.
//
// Returns: Returns an array of HostDocument objects

void HostApp::getDocuments(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_frame.isNull())
    {
        retval->setNull();
        return;
    }
    
    IDocumentSiteEnumPtr sites = m_frame->getShownDocumentSites(sitetypeNormal);
    
    retval->setArray(env);
    
    size_t i, count = sites->size();
    for (i = 0; i < count; ++i)
    {
        HostDocument* doc = HostDocument::createObject(env);
        doc->m_site = sites->getItem(i);
        
        kscript::Value val;
        val.setObject(doc);
        retval->appendMember(&val);
    }
}


// (METHOD) HostApp.getDocumentById
// Description: Returns an array of open documents in the program.
//
// Syntax: function HostApp.getDocumentById() : HostDocument
//
// Remarks: Each HostDocument object has an associated numeric ID.  This
//     number may be retrieved by calling HostDocument.getId().  The same
//     document can be looked up again by utilizing HostApp.getDocumentById().
//     If a document with the specified ID exists, this method will return
//     the corresponsing HostDocument object.  If not, |null| is returned.
//
// Returns: Returns a HostDocument object

void HostApp::getDocumentById(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_frame.isNull())
    {
        retval->setNull();
        return;
    }
    
    IDocumentSitePtr site = m_frame->lookupSiteById(env->getParam(0)->getInteger());
    
    HostDocument* doc = HostDocument::createObject(env);
    doc->m_site = site;
    retval->setObject(doc);
}




// (METHOD) HostApp.getActiveDocument
//
// Description: Returns the active document
//
// Syntax: function HostApp.getActiveDocument() : HostDocument
//
// Remarks: This method returns the active document in the system.  The
//     active document is the window that current has the user focus.  If
//     no documents are open in the system, null is returned.


void HostApp::getActiveDocument(kscript::ExprEnv* env, kscript::Value* retval)
{
    IDocumentSitePtr site = m_frame->getActiveChild();
    if (site.isNull())
    {
        retval->setNull();
        return;
    }
    
    HostDocument* doc = HostDocument::createObject(env);
    doc->m_site = site;
    
    retval->setObject(doc);
}






class HostAppContainerDoc : public IDocument
{
    XCM_CLASS_NAME("appmain.HostAppContainerDoc")
    XCM_BEGIN_INTERFACE_MAP(HostAppContainerDoc)
        XCM_INTERFACE_ENTRY(IDocument)
    XCM_END_INTERFACE_MAP()

public:

    HostAppContainerDoc(Form* form)
    {
        m_form = form;
        m_form->baseRef();
        m_doc_window = m_form->getFormPanel();
    }
    
    virtual ~HostAppContainerDoc()
    {
        m_form->baseUnref();
    }

    // -- IDocument --
    bool initDoc(IFramePtr frame,
                 IDocumentSitePtr doc_site,
                 wxWindow* docsite_wnd,
                 wxWindow* panesite_wnd)
    {
        m_form->setFormWindow(docsite_wnd);
        m_form->realize(); // this reparents the window
        
        wxSize size = docsite_wnd->GetClientSize();
        m_form->setPositionInt(0, 0, size.x, size.y);

        return true;
    }
    
    wxWindow* getDocumentWindow()
    {
        return m_doc_window;
    }
    
    wxString getDocumentTitle()
    {
        return wxEmptyString;
    }
    
    wxString getDocumentLocation()
    {
        return wxEmptyString;
    }
    
    void setDocumentFocus()
    {
        m_doc_window->SetFocus();
    }
    
    bool onSiteClosing(bool force) { return true; }
    void onSiteActivated() { }

private:

    Form* m_form;
    wxWindow* m_doc_window;
};



void HostApp::createDocument(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 ||
        !env->getParam(0)->isObject() ||
        !env->getParam(0)->getObject()->isKindOf(L"Form"))
    {
        retval->setBoolean(false);
        return;
    }
    
    Form* form = (Form*)env->getParam(0)->getObject();
    HostAppContainerDoc* doc = new HostAppContainerDoc(form);
    
    IDocumentSitePtr site;
    site = g_app->getMainFrame()->createSite(doc, sitetypeNormal, -1, -1, -1, -1);
    form->setSite(site);
}

void HostApp::createPane(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 ||
        !env->getParam(0)->isObject() ||
        !env->getParam(0)->getObject()->isKindOf(L"Form"))
    {
        retval->setBoolean(false);
        return;
    }
    
    
    int dock = dockFloating;
    
    if (env->getParamCount() <= 2)
    {
        switch (env->getParam(1)->getInteger())
        {
            case DockLeft: dock = dockLeft; break;
            case DockTop: dock = dockTop; break;
            case DockRight: dock = dockRight; break;
            case DockBottom: dock = dockBottom; break;
            case DockFloating: dock = dockFloating; break;
        }
    }    


    Form* form = (Form*)env->getParam(0)->getObject();
    HostAppContainerDoc* doc = new HostAppContainerDoc(form);

    // clamp the form width to positive values 
    // (-1 is used internally so the form expands 
    // to the maximum size, but don't allow this 
    // in the interface)
    int dock_width = (form->m_width >= 0 ? form->m_width : 0);
    int dock_height = (form->m_height >= 0 ? form->m_height : 0);

    // create the docked site
    IDocumentSitePtr site;
    site = g_app->getMainFrame()->createSite(doc,
                                             sitetypeDockable | dock,
                                             form->m_x, form->m_y,
                                             dock_width, dock_height);
    g_app->getMainFrame()->getAuiManager().GetPane(form->getFormPanel()).Caption(wxT("x")).FloatingSize(dock_width, dock_height);
    form->setSite(site);

    m_frame->refreshFrameLayout();
    
    
    HostDocument* docres = HostDocument::createObject(env);
    docres->m_site = site;
    retval->setObject(docres);
}



void HostApp::showPane(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 ||
        !env->getParam(0)->isObject())
    {
        retval->setBoolean(false);
        return;
    }
    
    bool visible = true;
    
    if (env->getParamCount() > 1)
        visible = env->getParam(1)->getBoolean();
    
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    if (!obj->isKindOf(L"HostDocument"))
        return;
        
    HostDocument* host_doc = (HostDocument*)obj;
    if (host_doc->m_site.isNull())
        return;
        
    IDocumentPtr doc = host_doc->m_site->getDocument();
    if (doc.isNull())
        return;
    
    wxAuiManager& mgr = g_app->getMainFrame()->getAuiManager();
    wxAuiPaneInfo& info = mgr.GetPane(host_doc->m_site->getContainerWindow());
    if (info.IsOk())
    {
        info.Show(visible);
        mgr.Update();
    }
}

// (METHOD) HostApp.crash
//
// Description: Simulates a host application crash
//
// Syntax: function HostApp.crash()
//
// Remarks: Calling crash() will simulate a general protection fault, which will
//     cause the program to crash.  This is useful for testing script applications
//     which have recovery mechanisms for this eventuality.


void HostApp::crash(kscript::ExprEnv* env, kscript::Value* retval)
{
    // cause a segmentation fault
    unsigned char* p = (unsigned char*)0;
    *p = 0xff;
}


// (METHOD) HostApp.hang
//
// Description: Simulates a host application crash
//
// Syntax: function HostApp.hang()
//
// Remarks: Calling hang() will simulate an application freeze by blocking the
//     main GUI thread.  This is useful for testing script applications
//     which have recovery mechanisms for this eventuality.

void HostApp::hang(kscript::ExprEnv* env, kscript::Value* retval)
{
    while (1)
    {
        kl::Thread::sleep(10000);
    }
}


// (METHOD) HostApp.exit
//
// Description: Causes the host application to exit
//
// Syntax: function HostApp.exit(force : Boolean)
//
// Remarks: Calling exit() causes the host application to exit.  The |force|
//     parameter, which may optionally be specified, indicates whether the
//     application should force closing.  If exiting is forced, the application
//     will not prompt the user to save unsaved documents.  The default value
//     for |force|, false, is assumed if the |force| parameter is not
//     specified.  In this case, the application will display prompt dialogs
//     to save modified documents.
//
// Param(force): If true, save dialogs will be suppressed and exiting will
//     happen immediately. 

void HostApp::exit(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_frame.isNull())
        return;

    if (env->getParamCount() > 0 && env->getParam(0)->getBoolean())
    {
        //m_frame->closeAll(true);
        //g_app->getMainWindow()->Destroy();
        m_frame->getFrameWindow()->Close(true);
    }
     else
    {
        m_frame->getFrameWindow()->Close(false);
    }
}


// DO NOT DOCUMENT - deprecated
// HostJob.system() - execute a system command (run a program, etc.)
//
//   Returns: (not defined)

void HostApp::system(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
        return;
        
    ::wxExecute(towx(env->getParam(0)->getString()));
}


// (METHOD) HostApp.refresh
//
// Description: Refreshes the host application's panels
//
// Syntax: function HostApp.refresh()
//
// Remarks: During the execution of the script, if new database objects
//     are created, removed, or modified, it might become necessary to
//     refresh the host application's panels, most notably the project panel.
//     This allows new database objects to be displayed in the project
//     panel, and makes deleted ones disappear.


void HostApp::refresh(kscript::ExprEnv* env, kscript::Value* retval)
{
    g_app->getAppController()->refreshDbDoc();
    retval->setUndefined();
}


// (METHOD) HostApp.checkVersion
//
// Description: Checks the version number of the host application
//
// Syntax: function HostApp.checkVersion(major : Number,
//                                       minor : Number,
//                                       subminor : Number,
//                                       build_serial : Number) : Boolean               
//
// Remarks: Calling checkVersion() checks the version number specified
//     as four parameters against the host application's version number.
//     If the host application's version number is greater or equal
//     to the specified version, the function returns true.  Otherwise, it
//     returns false.  This is useful for scripts that rely on certain
//     features only present in newer versions of the host app.
//
// Param(major): The major version number (zero if not specified)
// Param(minor): The minor version number (zero if not specified)
// Param(subminor): The sub-minor version number (zero if not specified)
// Param(build_serial): The build serial number (zero if not specified)
// Returns: True if the host application is equal or newer to the specified
//     version number

void HostApp::checkVersion(kscript::ExprEnv* env, kscript::Value* retval)
{
    int v1 = 0, v2 = 0, v3 = 0, v4 = 0;
    
    if (env->m_param_count > 0)
        v1 = env->getParam(0)->getInteger();
    if (env->m_param_count > 1)
        v2 = env->getParam(1)->getInteger();
    if (env->m_param_count > 2)
        v3 = env->getParam(2)->getInteger();
    if (env->m_param_count > 3)
        v4 = env->getParam(3)->getInteger();
    
    bool result = true;
    
    if (APP_VERSION_MAJOR >= v1)
    {
        if (APP_VERSION_MINOR >= v2)
        {
            if (APP_VERSION_SUBMINOR >= v3)
            {
                if (APP_VERSION_BUILDSERIAL >= v4)
                {
                }
                 else
                {
                   result = false;
                }
            }
             else
            {
                result = false;
            }
        }
         else
        {
            result = false;
        }
    }  
     else
    {
        result = false;
    }
    
    retval->setBoolean(result);
}


// (METHOD) HostApp.getVersion
//
// Description: Returns the version of the host application as a string
//
// Syntax: function HostApp.getVersion() : String
//
// Remarks: Returns the version of the host application as a string. This
//     string is formatted as four period-separated integers
//     (for example, 1.2.3.4).  The numbers mean, from left to right, major
//     version number, minor version number, sub-minor version number, and
//     build serial.  The individual elements can be parsed out using the
//     String functions.
//
// Returns: The host application's version number


void HostApp::getVersion(kscript::ExprEnv* env, kscript::Value* retval)
{
    wxString s = wxString::Format(wxT("%d.%d.%d.%d"),
                             APP_VERSION_MAJOR,
                             APP_VERSION_MINOR,
                             APP_VERSION_SUBMINOR,
                             APP_VERSION_BUILDSERIAL);
    
    retval->setString(towstr(s).c_str());
}

// (METHOD) HostApp.getApplicationId
//
// Description: Returns the application identification tag
//
// Syntax: function HostApp.getApplicationId() : String
//
// Remarks: Returns an identification tag that corresponds to the
//     host application.
//
// Returns: The application identification tag


void HostApp::getApplicationId(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setString(PALADIN_APP_TAG);
}




// DO NOT DOCUMENT - deprecated
void HostApp::installExtensionMainMenu(kscript::ExprEnv* env, kscript::Value* retval)
{
}

// DO NOT DOCUMENT - deprecated
void HostApp::installExtensionMenu(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
        return;

    if (m_frame.isNull())
        return;
        
    wxMenuBar* menu_bar = m_frame->getFrameWindow()->GetMenuBar();

    if (!menu_bar)
        return;
    
    if (env->getParam(0)->isObject())
    { 
        kscript::ValueObject* obj = (kscript::ValueObject*)env->getParam(0)->getObject();
        if (obj->getClassName() == L"MenuItem")
        {
            MenuItem* item = (MenuItem*)obj;
            
            int idx = menu_bar->FindMenu(_("Tools"));
            if (idx != wxNOT_FOUND)
            {
                wxMenu* menu = menu_bar->GetMenu(idx);
                
                item->setWxMenu(menu);
                menu->Append(item->getWxMenuItem());
            }
        }
    }
}



// (METHOD) HostApp.executeAndWait
//
// Description: Executes a script, template, or executable object
//     in the host application in synchronous fashion
//
// Syntax: function HostApp.executeAndWait(path : String) : Boolean
//
// Remarks: The executeAndWait() method allows the caller to execute a script,
//     template, or executable object in the project panel.  The execution
//     occurs asynchronously, meaning that code execution returns immediately
//     to the calling scope, while the executable specified by |path| is
//     executed simultaneously.
//
// Param(path): The path or url of the script, template, or executable object
// Returns: True upon success, false otherwise

void HostApp::executeAndWait(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setBoolean(false);
        return;
    }
    
    // construct a flags parameter
    kscript::Value flags;
    flags.setInteger(0);
    
    if (env->getParamCount() >= 2)
    {
        flags.setInteger(env->getParam(1)->getInteger());
    }
    
    // add the HostApp.ExecuteWait flag
    flags.setInteger(flags.getInteger() | HostApp::ExecuteWait);
    
    kscript::ExprEnv e;
    e.m_parser = env->m_parser;
    e.reserveParams(2);
    e.m_param_count = 2;
    e.m_eval_params[0] = env->m_eval_params[0];
    e.m_eval_params[1] = &flags;
    
    execute(&e, retval);
    
    e.m_eval_params[0] = NULL;
    e.m_eval_params[1] = NULL;
}




class ExecuteJobRunner : public wxEvtHandler
{
public:

    ExecuteJobRunner()
    {
        m_ready = 0;
        m_flags = 0;
    }
    
    bool ProcessEvent(wxEvent& evt)
    {   
        IJobPtr job;
        
        if (m_flags & HostApp::ExecuteSource)
        {
            job = g_app->getAppController()->executeCode(m_target);
        }
         else if (m_flags & HostApp::ExecuteTemplate)
        {
            // get database ptr
            tango::IDatabasePtr db = g_app->getDatabase();
            if (db.isNull())
                return 0;
                
            // generate temporary path for template
            wxString path = wxT("/.temp/template_");
            path += makeUniqueString();
            
            // get template mime type
            kl::JsonNode node;
            if (!node.fromString(towstr(m_target)))
                return 0;
            kl::JsonNode metadata = node["metadata"];
            wxString mime_type = metadata["type"];
            
            // write out temporary template
            writeStreamTextFile(db, towstr(path), towstr(m_target), towstr(mime_type));
            
            job = g_app->getAppController()->execute(path);
            
            // delete template template
            db->deleteFile(towstr(path));
        }
         else
        {
            job = g_app->getAppController()->execute(m_target);
        }
        
        if (job.isOk())
        {
            m_job_info = job->getJobInfo();
        }
        
        m_ready = 1;
        
        return true;
    }
    
public:

    wxString m_target;
    int m_flags;
    IJobInfoPtr m_job_info;
    
    int m_ready;
};


// (METHOD) HostApp.execute
//
// Description: Executes a script, template, or executable object
//     in the host application
//
// Syntax: function HostApp.execute(target : String, flags : Number) : Boolean
//
// Remarks: The execute() method allows the caller to execute a script,
//     template, or executable object in the project panel.  By default, the execution
//     occurs synchronously, meaning that code execution returns immediately
//     to the calling scope, while the executable specified by |path| is
//     executed simultaneously.  In the |flags| parameter, one or more flags
//     may be specified which alter the behavior of the call.  If the user specifies
//     ExecuteWait, the execution will run asychronously.  If the ExecuteSource flag
//     is specified, |target| must contain the source code that is to be executed.
//
// Param(target): The path, url, or source code of the script, template, or executable object
// Param(flags): Optional flags changing the execution behavior.
// Returns: True upon success, false otherwise

void HostApp::execute(kscript::ExprEnv* env, kscript::Value* retval)
{    
    retval->setBoolean(false);
    
    if (env->getParamCount() < 1)
        return;
    
    int flags = 0;
    
    ExecuteJobRunner* r = new ExecuteJobRunner;
    r->m_target = towx(env->getParam(0)->getString());
    flags = 0;
    if (env->getParamCount() >= 2)
        flags = env->getParam(1)->getInteger();
    r->m_flags = flags;
    
    if (flags & HostApp::ExecuteTemplate)
    {
        if (env->getParam(0)->isObject() && r->m_target.Left(1) == wxT("["))
        {
            // we need to stringify the object into json ourselves;
            // TODO: any way to use kl::JsonNode?
            kscript::JsonNode node(*env->getParam(0));
            r->m_target = node.toString();
        }
    }
    
    wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, 5000);
    wxPostEvent(r, evt);

    // wait for job to get started
    int wait = 1;
    while (!r->m_ready)
    {
        if (wait < 50)
            wait++;
        kl::Thread::sleep(wait);
    }
    
    IJobInfoPtr job_info = r->m_job_info;
    
    delete r;
    r = NULL;
    
    if (job_info.isNull())
        return;
        
    // if the wait flag is on, we need to wait for the job to finish
    if (flags & HostApp::ExecuteWait)
    {
        // wait for job to get started
        int wait = 100;
        do
        {
            if (wait < 1000)
                wait++;
            kl::Thread::sleep(wait);
        } while (job_info->getState() == jobStateRunning);
    }
     else
    {
        if (job_info.isOk())
            retval->setBoolean(true);
    }
}


// (METHOD) HostApp.getCommandLineArguments
//
// Description: Returns an array of command line arguments
//
// Syntax: function HostApp.getCommandLineArguments() : Array(String)
//
// Remarks: This method retrieves the command line arguments that were
//     specified upon program execution.
//
// Returns: An array of strings containing the command line parameters

void HostApp::getCommandLineArguments(kscript::ExprEnv* env, kscript::Value* retval)
{
    int start_arg = 0;
    int end_arg = g_app->argc;
    int i;
 
    // skip host executable filename
    start_arg++;
    
    retval->setArray(env);
    for (i = start_arg; i < end_arg; ++i)
    {
        {
            kscript::Value v;
            v.setString(g_app->argv[i]);
            retval->appendMember(&v);
        }
    }
}


// (METHOD) HostApp.isGuiActive
//
// Description: Returns true if a user interface is present
//
// Syntax: function HostApp.isGuiActive() : Boolean
//
// Remarks: This method returns true if a graphical user interface
//     is being used to run the software.  False is returned if
//     console mode is being used.
//
// Returns: True if a gui is active, false otherwise
    
void HostApp::isGuiActive(kscript::ExprEnv* env, kscript::Value* retval)
{
    #if APP_CONSOLE==1
    retval->setBoolean(false);
    #else
    retval->setBoolean(true);
    #endif
}

// (METHOD) HostApp.isService
//
// Description: Returns true if the software is running in service mode
//
// Syntax: function HostApp.isService() : Boolean
//
// Remarks: This method returns true is the software is running
//     as a service.   If the software is run as a desktop application
//     with a graphical user interface, the method returns false.
//
// Returns: True if the software is being run as a service, false otherwise
    
void HostApp::isService(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(g_app->isService());
}


// HostApp::evalExpression is private for testing: don't document
void HostApp::evalExpression(kscript::ExprEnv* env, kscript::Value* retval)
{
    // default return value
    retval->setNull();

    // TODO: ineffecient to create a parser each time; is there
    // a better way?
    kscript::ExprParser* expr_parser = NULL;
    expr_parser = new kscript::ExprParser(kscript::optionLanguageGeneric |
                                          kscript::optionEpsilonNumericCompare);

    if (expr_parser == NULL)
        return;

    // this line adds standard database functions to the expression parser
    // (for example SUBSTR())
    tango::bindExprParser((void*)expr_parser);

    // parse and evaluate the expression
    if (env->getParamCount() > 0)
    {
        // if the expression doesn't parse, raise an exception
        std::wstring expr = env->getParam(0)->getString();
        if (!expr_parser->parse(expr.c_str()))
        {
            kscript::Value val;
            env->setRuntimeError(env->createObject(L"Error", &val));
        }
        else
        {
            // if evaluating the expression returns an error,
            // raise an exception
            int result = expr_parser->eval(retval);
            if (result < 0)
            {
                kscript::Value val;
                env->setRuntimeError(env->createObject(L"Error", &val));
            }
        }
    }

    delete expr_parser;
}

Application* HostApp::getApp()
{
    Application* ret = m_app.getApp();
    return ret;
}

size_t HostApp::invokeJsEvent(const std::wstring& evt,
                              kscript::Value* event_args,
                              unsigned int event_flags)
{
    kscript::Value* vevt = getMember(evt);
    if (!vevt)
        return 0;

    Event* e = (Event*)vevt->getObject();
    
    // if there are no receivers for the event,
    // don't even fire it
    size_t sink_count = e->getSinkCountInternal();
    if (sink_count == 0)
    {
        delete event_args;
        return 0;
    }
        
    Application* app = getApp();
    if (app)
    {
        app->postEvent(e, this, event_args, event_flags);
    }
    
    return sink_count;
}

void HostApp::onActiveChildChanged(IDocumentSitePtr doc_site)
{
    // -- get the name of the child --
    if (doc_site.isOk())
    {
        IDocumentPtr doc = doc_site->getDocument();
        if (doc.isOk())
        {
            wxString location, caption;
            location = doc->getDocumentLocation();
            caption = doc_site->getCaption();
            
            // fire the location changed event if the new
            // active child has a different location than
            // the old one
            if (location != m_location)
            {
                m_location = location;
                
                kscript::Value* event_args1 = new kscript::Value;
                event_args1->setObject();
                event_args1->getMember(L"location")->setString(location);
                event_args1->getMember(L"caption")->setString(caption);
                invokeJsEvent(L"locationChanged", event_args1);
            }
            
            // fire the active child changed event
            kscript::Value* event_args2 = new kscript::Value;
            event_args2->setObject();
            event_args2->getMember(L"location")->setString(location);
            event_args2->getMember(L"caption")->setString(caption);
            invokeJsEvent(L"activeChildChanged", event_args2);
            return;
        } 
    }
}

void HostApp::onFrameEvent(FrameworkEvent& evt)
{
    if (evt.name == FRAMEWORK_EVT_CONSOLEPANEL_COMMAND)
    {
        wxString console_command = evt.s_param;

        kscript::Value* event_args = new kscript::Value;
        event_args->setObject();
        event_args->getMember(L"value")->setString(console_command);
        invokeJsEvent(L"commandEntered", event_args);

        return;
    }

    if (evt.name == FRAMEWORK_EVT_CFW_LOCATION_CHANGED)
    {
        IDocumentSitePtr doc_site = m_frame->getActiveChild();
        if (doc_site.isOk())
        {
            IDocumentPtr doc = doc_site->getDocument();
            if (doc.isOk())
            {
                wxString location, caption;
                location = doc->getDocumentLocation();
                caption = doc_site->getCaption();

                // fire the location changed event if the new
                // active child has a different location than
                // the old one
                if (location != m_location)
                {
                    m_location = location;
                    
                    kscript::Value* event_args = new kscript::Value;
                    event_args->setObject();
                    event_args->getMember(L"location")->setString(location);
                    event_args->getMember(L"caption")->setString(caption);
                    invokeJsEvent(L"locationChanged", event_args);
                }
                
                return;
            }
        }
    }
}

void HostApp::onFrameDestroy()
{
    invokeJsEvent(L"frameDestroyed");
    m_frame.clear();
}









// (CLASS) HostDocument
// Category: HostApp
// Description: A class that exposes method calls to manipulate document windows.
// Remarks: A HostDocument object represents an application document object
//     inside the host application.  Information such as the document's
//     caption or document location can be retrieved.


// (METHOD) HostDocument.setCaption
//
// Description: Changes the document's caption
//
// Syntax: function HostApp.setCaption(caption : String) : Boolean
//
// Remarks: Calling setCaption will update the caption that is displayed
//     for the document.
//
// Param(caption): A string containing the new caption
// Returns: True upon success, false otherwise


void HostDocument::setCaption(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_site.isNull() || env->getParamCount() < 1)
    {
        retval->setBoolean(false);
        return;
    }
    
    m_site->setCaption(towx(env->getParam(0)->getString()));
    retval->setBoolean(true);
}




// (METHOD) HostDocument.getCaption
//
// Description: Returns the document's current caption
//
// Syntax: function HostApp.getCaption() : String
//
// Remarks: Returns the document's current caption.  If the document is no
//     longer open, null is returned
//
// Returns: A string containing the caption, or null upon error

void HostDocument::getCaption(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_site.isNull())
    {
        retval->setNull();
        return;
    }
    
    retval->setString(towstr(m_site->getCaption()));
}


// (METHOD) HostDocument.getLocation
//
// Description: Returns the document's location
//
// Syntax: function HostApp.getLocation() : String
//
// Remarks: Returns the document's location.  This can be a internal
//     table name, a filename, or a url.
//
// Returns: A string containing the document location, or null upon error


void HostDocument::getLocation(kscript::ExprEnv* env, kscript::Value* retval)
{
    IDocumentPtr doc;
    if (m_site.isOk())
        doc = m_site->getDocument();
        
    if (doc.isNull())
    {
        retval->setNull();
        return;
    }

    retval->setString(towstr(doc->getDocumentLocation()));
}

// TODO: document
// this now belongs in WebDoc
void HostDocument::getDOMDocument(kscript::ExprEnv* env, kscript::Value* retval)
{
    // default return value
    retval->setNull();

    // get the document
    IDocumentPtr doc;
    if (m_site.isOk())
        doc = m_site->getDocument();

    // if the document is null, we're done
    if (doc.isNull())
        return;

    // get the web document; if it's null, we're done
    IWebDocPtr webdoc = doc;
    if (webdoc.isNull())
        return;
        
    wxWebControl* webcontrol = webdoc->getWebControl();
    if (webcontrol == NULL)
        return;

    wxDOMDocument result = webcontrol->GetDOMDocument();
    if (result.IsOk())
    {
        WebDOMDocument* res = WebDOMDocument::createObject(env);
        res->m_node = result;
        retval->setObject(res);
    } 

    return;
}


// (METHOD) HostDocument.getType
//
// Description: Returns the document's location
//
// Syntax: function HostApp.getType() : String
//
// Remarks: Returns the document's type as a string  This value may be "table",
//     "editor", "query", "report", or "web".  If the document fails (perhaps because
//     the document no longer exists), null is returned
//
// Returns: A string containing the document type, or null upon error


void HostDocument::getType(kscript::ExprEnv* env, kscript::Value* retval)
{
    // default return value
    retval->setNull();

    // get the document
    IDocumentPtr doc;
    if (m_site.isOk())
        doc = m_site->getDocument();

    // if the document is null, we're done
    if (doc.isNull())
        return;

    ITableDocPtr t = doc;
    IEditorDocPtr e = doc;
    IQueryDocPtr q = doc;
    IReportDocPtr r = doc;
    IWebDocPtr w = doc;
    
    if (t.isOk())
        retval->setString(L"table");
    else if (e.isOk())
        retval->setString(L"editor");
    else if (q.isOk())
        retval->setString(L"query");
    else if (r.isOk())
        retval->setString(L"report");
    else if (w.isOk())
        retval->setString(L"web");
}

void HostDocument::getId(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_site)
        retval->setInteger(m_site->getId());
         else
        retval->setInteger(0);
}

kscript::Value* HostDocument::getMember(const std::wstring& name)
{
    // get the document
    IDocumentPtr doc;
    if (m_site)
    {
        doc = m_site->getDocument();
        if (doc)
        {
            IDocumentScriptBindingPtr doc_script;
            doc_script = doc;
            if (doc_script)
            {
                kscript::Value retval;
                bool result = doc_script->getScriptMember(name, &retval);
                if (result)
                {
                    getValueReturn()->setValue(retval);
                    return getValueReturn();
                }
            }
        }
    }
    
    return ValueObject::getMember(name);
}



// HostJob is a wrapper around m_job_info.  It also facilitates
// adding a job to the queue.  By default, the current count is 0
// and the maximum count is 100

HostJob::HostJob()
{
    m_job_info = jobs::createJobInfoObject();
    
    m_job_info->setTitle(wxT(""));
    m_job_info->setMaxCount(100);
    
    m_job_info->setInfoMask(jobMaskTitle |
                        jobMaskStartTime |
                        jobMaskFinishTime |
                        jobMaskPercentage |
                        jobMaskProgressString |
                        jobMaskProgressBar |
                        jobMaskCurrentCount);
}

HostJob::~HostJob()
{
}

void HostJob::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}


// HostJob.cancel() - changes job state from 'Cancelling' to 'Cancelled'
//
//   Returns: (not defined)

void HostJob::cancel(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_job_info->setFinishTime(time(NULL));
    m_job_info->setState(jobStateCancelled);
}


// HostJob.isCancelling() - returns cancel state
//
//   Returns: Boolean true if the user has pressed the cancel button; state = 'Cancelling'

void HostJob::isCancelling(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(m_job_info->getState() == jobStateCancelling ? true : false);
}


// HostJob.start() - sets the job state to 'Running'
//
//   Returns: (not defined)

void HostJob::start(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_job_info->setStartTime(time(NULL));
    g_app->getJobQueue()->addJobInfo(m_job_info, jobStateRunning);
}


// HostJob.finish() - sets the job state to 'Finished'
//
//   Returns: (not defined)

void HostJob::finish(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_job_info->setFinishTime(time(NULL));
    m_job_info->setState(jobStateFinished);
}


// HostJob.getMaximum() - returns the maximum count
//
//   Returns: a numeric value indicating the maximum count

void HostJob::getMaximum(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setDouble(m_job_info->getMaxCount());
}


// HostJob.getValue() - returns the current count
//
//   Returns: a numeric value indicating the current count

void HostJob::getValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setDouble(m_job_info->getCurrentCount());
}


// HostJob.incrementValue(Double) - increments the current count
//
//   Parameter 1: Value to add to the current count.  If not specified,
//                the current count will be incremented by 1.
//   Returns: (not defined)

void HostJob::incrementValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() > 0)
    {
        m_job_info->incrementCurrentCount(env->getParam(0)->getDouble());
    }
     else
    {
        m_job_info->incrementCurrentCount(1.0);
    }
}


// HostJob.setTitle(String) - sets job title
//
//   Parameter 1: New title for the job info entry
//   Returns: (not defined)

void HostJob::setTitle(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() > 0)
    {
        m_job_info->setTitle(env->getParam(0)->getString());
    }
}


// HostJob.setMaximum(String) - sets new maximum count
//
//   Parameter 1: New maximum count
//   Returns: (not defined)

void HostJob::setMaximum(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() > 0)
    {
        m_job_info->setMaxCount(env->getParam(0)->getDouble());
    }
}


// HostJob.setValue(Double) - sets new current count
//
//   Parameter 1: New value for the current count
//   Returns: (not defined)

void HostJob::setValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() > 0)
    {
        m_job_info->setCurrentCount(env->getParam(0)->getDouble());
    }
}




// -- HostGlobal class implementation --
class HostGlobalData
{
public:

    HostGlobalData()
    {
    }
    
    ~HostGlobalData()
    {
        std::map<std::wstring, kscript::Value*>::iterator it;
        for (it = m_map.begin(); it != m_map.end(); ++it)
            delete it->second;
    }
    
    void lock()
    {
        m_mutex.lock();
    }
    
    void unlock()
    {
        m_mutex.unlock();
    }
    
    kscript::Value* getEntry(const std::wstring& name)
    {
        std::map<std::wstring, kscript::Value*>::iterator it;
        it = m_map.find(name);
        if (it != m_map.end())
            return it->second;
        
        kscript::Value* v = new kscript::Value;
        m_map[name] = v;
        return v;
    }
    
private:

    std::map<std::wstring, kscript::Value*> m_map;
    xcm::mutex m_mutex;
};




HostGlobalData g_host_global_data;


HostGlobal::HostGlobal()
{
}

HostGlobal::~HostGlobal()
{
}

void HostGlobal::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

kscript::Value* HostGlobal::getMember(const std::wstring& name)
{
    g_host_global_data.lock();
    kscript::Value* v = g_host_global_data.getEntry(name);
    kscript::Value* ret = getValueReturn();
    ret->setValue(v);
    g_host_global_data.unlock();
    return ret;
}

void HostGlobal::setMember(const std::wstring& name, kscript::Value* value)
{
    if (value->isObject())
    {
        getParser()->setRuntimeError(kscript::rterrorTypeError, NULL, L"");
        return;
    }
    
    g_host_global_data.lock();
    kscript::Value* v = g_host_global_data.getEntry(name);
    v->setValue(value);
    g_host_global_data.unlock();
}





// -- HostServices class implementation --


HostServices::HostServices()
{
}

HostServices::~HostServices()
{
    std::map<std::wstring, kscript::ValueObject*>::iterator it;
    for (it = m_services.begin(); it != m_services.end(); ++it)
    {
        if (it->second)
            it->second->baseUnref();
    }
}

void HostServices::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

void HostServices::getServiceInternal(
                                  const std::wstring service_name,
                                  kscript::Value* retval)
{
    retval->setNull();
    
    std::map<std::wstring, kscript::ValueObject*>::iterator it;
    it = m_services.find(service_name);
    if (it != m_services.end())
    {
        retval->setObject(it->second);
        return;
    }
    
    
    kscript::ValueObject* obj = NULL;
    
    if (service_name == L"preferences")
    {
        obj = static_cast<kscript::ValueObject*>(HostPreferences::createObject(getParser()));
    }
     else if (service_name == L"data")
    {
        obj = static_cast<kscript::ValueObject*>(HostData::createObject(getParser()));
    }
     else if (service_name == L"printing")
    {
        obj = static_cast<kscript::ValueObject*>(HostPrinting::createObject(getParser()));
    }
     else if (service_name == L"automation")
    {
        obj = static_cast<kscript::ValueObject*>(HostAutomation::createObject(getParser()));
    }
        
    if (obj)
    {
        obj->baseRef();
        m_services[service_name] = obj;
        retval->setObject(obj);
    }
}

    

void HostServices::get(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setNull();
        return;
    }
            
    getServiceInternal(env->getParam(0)->getString(), retval);
}


kscript::Value* HostServices::getMember(const std::wstring& name)
{
    kscript::Value* v = getValueReturn();
    getServiceInternal(name, v);
    return v;
}





// -- HostBitmap class implementation --


HostBitmap::HostBitmap()
{
}

HostBitmap::~HostBitmap()
{
}

void HostBitmap::getBitmapInternal(kscript::ExprEnv* env,
                                  const std::wstring bitmap_name,
                                  const int bitmap_size,
                                  kscript::Value* retval)
{
    retval->setNull();
    
    wxString name = towx(bitmap_name);
    name.Prepend(wxT("gf_"));
    name += wxString::Format(wxT("_%d"), bitmap_size);
    
    wxBitmap bmp = BitmapMgr::getBitmap(name);
    if (!bmp.IsOk())
        return;
    
    Bitmap* b = Bitmap::createObject(env);
    b->setWxBitmap(bmp);
    retval->setObject(b);
}

    

void HostBitmap::get(kscript::ExprEnv* env, void*, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setNull();
        return;
    }
    
    std::wstring bitmap_name = env->getParam(0)->getString();
    int bitmap_size = 16;
    
    if (env->getParamCount() > 1)
        bitmap_size = env->getParam(1)->getInteger();
    
    getBitmapInternal(env, bitmap_name, bitmap_size, retval);
}




// -- HostPreferences class implementation --

// (CLASS) HostPreferences
// Category: HostApp
// Description: A class that provides a mechanism to store application preferences.
// Remarks: The HostPreferences class allows programs to store their
//     own user preferences (for example, saved form information) in the
//     application's preference database.  Preference key names normally have a
//     dotted namespace format such as 'program_name.component.preference_name',
//     and programs which use this class should follow this convention.

HostPreferences::HostPreferences()
{
}

HostPreferences::~HostPreferences()
{
}

void HostPreferences::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}


// (METHOD) HostPreferences.setValue
//
// Description: Sets a preference value in the application preference database
//
// Syntax: function HostPreferences.setValue(pref_name : String, 
//                                           value : Any) : Boolean
//
// Remarks: Sets the value of a preference in the application preference
//     database.  If the preference entry does not already exist, a new
//     entry is created.  If the preference entry does already exist, the
//     old value is replaced with the value passed to this method.  A string,
//     number, or boolean value may be passed in the |value| parameter.
//
// Returns: True if the preference was properly set, false otherwise

void HostPreferences::setValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
    
    if (env->getParamCount() < 2)
        return;
    
    IAppPreferencesPtr prefs = g_app->getAppPreferences();
    if (prefs.isNull())
        return;
    
    wxString pref_name = towx(env->getParam(0)->getString());
    
    kscript::Value* param2 = env->getParam(1);
    if (param2->isBoolean())
    {
        prefs->setBoolean(pref_name, param2->getBoolean());
    }
     else if (param2->isNumber())
    {
        prefs->setLong(pref_name, param2->getInteger());
    }
     else if (param2->isString())
    {
        prefs->setString(pref_name, towx(param2->getString()));
    }
     else
    {
        return;
    }
    
    prefs->flush();
    retval->setBoolean(true);
}

// (METHOD) HostPreferences.getValue
//
// Description: Retrieves a preference value from the application preference database
//
// Syntax: function HostPreferences.getValue(pref_name : String) : Any
//
// Remarks: Retrieves the value of the preference specified in the |pref_name|
//     parameter.  If the value doesn't exist in the application's preference
//     database, or the value could not be retrieved, |null| is returned.  Otherwise
//     either a string or numeric value is returned containing the preference
//     value.
//
// Returns: The value of the preference, or null if the preference
//     was not found or could not be retrieved.

void HostPreferences::getValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();
    
    if (env->getParamCount() < 1)
        return;
    
    IAppPreferencesPtr prefs = g_app->getAppPreferences();
    if (prefs.isNull())
        return;
        
    wxString pref_name = towx(env->getParam(0)->getString());
    
    if (!prefs->exists(pref_name))
        return;
    
    if (prefs->getType(pref_name) == AppPreference::typeLong)
    {
        retval->setInteger(prefs->getLong(pref_name, 0));
    }
     else
    {
        retval->setString(towstr(prefs->getString(pref_name, wxT(""))));
    }
}

// (METHOD) HostPreferences.deleteValue
//
// Description: Deletes a preference value from the application preference database
//
// Syntax: function HostPreferences.deleteValue(pref_name : String) : Boolean
//
// Remarks: Deletes the preference entry specified in the |pref_name|
//     parameter.  If the value doesn't exist in the application's preference
//     database, or the value could not be deleted, |false| is returned.  
//
// Returns: True if the preference was successfully deleted from the preference
//     database, false otherwise.

void HostPreferences::deleteValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
    
    if (env->getParamCount() < 1)
        return;
    
    IAppPreferencesPtr prefs = g_app->getAppPreferences();
    if (prefs.isNull())
        return;
        
    retval->setBoolean(prefs->remove(towx(env->getParam(0)->getString())));
}




// (METHOD) HostPreferences.exists
//
// Description: Checks for the existence of a preference
//
// Syntax: function HostPreferences.exists(pref_name : String) : Boolean
//
// Remarks: Checks if the preference specified in the |pref_name| parameter
//     exists in the application database.
//
// Returns: True if the preference exists, false otherwise.

void HostPreferences::exists(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
    
    if (env->getParamCount() < 1)
        return;
    
    IAppPreferencesPtr prefs = g_app->getAppPreferences();
    if (prefs.isNull())
        return;
        
    retval->setBoolean(prefs->exists(towx(env->getParam(0)->getString())));
}



// (METHOD) HostPreferences.getAll
//
// Description: Retrieves all preferences from the preference database
//
// Syntax: function HostPreferences.getAll() : Array(Number/String)
//
// Remarks: This method returns all preferences in the application preference
//     database.  The object returned is a |dictionary array| indexed by preference
//     name and containing the preference values.  See the example for more information
//     on using this method.
//
// Returns: An array containing all preferences, or null if the call failed


void HostPreferences::getAll(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();
    
    IAppPreferencesPtr prefs = g_app->getAppPreferences();
    if (prefs.isNull())
        return;
        
    retval->setArray(env);
    
    std::vector<AppPreference> pref_vec;
    std::vector<AppPreference>::iterator it;
    
    prefs->getAll(pref_vec);
    
    for (it = pref_vec.begin(); it != pref_vec.end(); ++it)
    {
        if (it->type == AppPreference::typeLong)
        {
            retval->getMember(towstr(it->pref))->setInteger(it->longval);
        }
         else
        {
            retval->getMember(towstr(it->pref))->setString(it->strval);
        }
    }
}






HostData::HostData()
{
}

HostData::~HostData()
{
}

void HostData::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}



class HostDataDefinitionField
{
public:
    std::wstring source_name;
    int source_offset;
    int source_width;
    int source_encoding;
    std::wstring name;
    int type;
    int width;
    int scale;
    std::wstring formula;
};

void HostData::assignDefinition(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
    
    std::wstring filename;
    
    if (env->getParamCount() < 2)
        return;
    
    filename = env->getParam(0)->getString();
    
    if (!env->getParam(1)->isObject())
        return;
    kscript::ValueObject* info = env->getParam(1)->getObject();
    if (!info->getMemberExists(L"type"))
        return;

    std::wstring type = info->getMember(L"type")->getString();
    
    if (type == L"fixed")
    {
        int start_offset = 0;
        int row_width = 0;
        std::wstring line_delimiters = L"";
        std::vector<HostDataDefinitionField> fields;
        
        if (info->getMemberExists(L"start_offset"))
            start_offset = info->getMember(L"start_offset")->getInteger();
        if (info->getMemberExists(L"row_width"))
            row_width = info->getMember(L"row_width")->getInteger();
        if (info->getMemberExists(L"line_delimiters"))
            line_delimiters = info->getMember(L"line_delimiters")->getString();
            
        if (info->getMemberExists(L"fields") && info->getMember(L"fields")->isObject())
        {
            kscript::ValueObject* fields_obj = info->getMember(L"fields")->getObject();
            size_t i, count = fields_obj->getRawMemberCount();
            for (i = 0; i < count; ++i)
            {
                kscript::Value* mval = fields_obj->getRawMemberByIdx(i);
                if (mval && mval->isObject())
                {
                    HostDataDefinitionField field;
                    field.source_offset = 0;
                    field.source_width = 0;
                    field.source_encoding = 0;
                    field.name = L"";
                    field.type = 0;
                    field.width = 0;
                    field.scale = 0;
                    
                    int source_offset = 0, source_width = 0;
                    std::wstring name;
                    int type = 0, width = 0, scale = 0;

                    if (mval->getMemberExists(L"source_offset"))
                        field.source_offset = mval->getMember(L"source_offset")->getInteger();
                    if (mval->getMemberExists(L"source_width"))
                        field.source_width = mval->getMember(L"source_width")->getInteger();
                    if (mval->getMemberExists(L"source_encoding"))
                        field.source_encoding = DbEncoding::toTangoEncoding(mval->getMember(L"source_encoding")->getInteger());
                    if (mval->getMemberExists(L"name"))
                        field.name = mval->getMember(L"name")->getString();
                    if (mval->getMemberExists(L"type"))
                        field.type = DbType::toTangoType(mval->getMember(L"type"));
                    if (mval->getMemberExists(L"width"))
                        field.width = mval->getMember(L"width")->getInteger();
                    if (mval->getMemberExists(L"scale"))
                        field.scale = mval->getMember(L"scale")->getInteger();
                    
                    
                    // get source name; if there's no source name, use the destination name
                    if (mval->getMemberExists(L"source_name"))
                        field.source_name = mval->getMember(L"source_name")->getString();
                         else
                        field.source_name = field.name;

                    // get formula; if there's no formula, use the source name
                    if (mval->getMemberExists(L"formula"))
                        field.formula = mval->getMember(L"formula")->getString();
                         else
                        field.formula = field.source_name;
                    kl::replaceStrNoCase(field.formula, L"$SRCFIELD", field.source_name);

                    fields.push_back(field);
                }
            }


            // -- now apply the settings to the file --
    
            std::wstring set_path;
            if (filename.substr(0,5) != L"file:")
            {
                if (xf_get_file_exist(filename))
                    set_path = kl::filenameToUrl(filename);
                     else
                    set_path = filename;
            }
             else
            {
                set_path = filename;
            }
            
            tango::IFixedLengthDefinitionPtr set;
            set = g_app->getDatabase()->openSetEx(set_path, tango::formatFixedLengthText);
            if (!set)
                return;
            
            
            set->setRowWidth(row_width);
            set->setBeginningSkipCharacterCount(start_offset);
            
            // FIXME: the tango interface needs to be fixed to
            // accept an actual list of line delimiters; right now
            // "\n" is assumed
            if (line_delimiters.length() > 0)
                set->setLineDelimited(true);
                 else
                set->setLineDelimited(false);
            
            
            tango::IStructurePtr s;
            size_t col_count;
            std::vector<HostDataDefinitionField>::iterator it;

            // delete all existing source fields
            s = set->getSourceStructure();
            col_count = s->getColumnCount();
            for (i = 0; i < col_count; ++i)
                s->deleteColumn(s->getColumnName(i));
            set->modifySourceStructure(s, NULL);
            
            // delete all existing destination fields
            s = set->getDestinationStructure();
            col_count = s->getColumnCount();
            for (i = 0; i < col_count; ++i)
                s->deleteColumn(s->getColumnName(i));
            set->modifyDestinationStructure(s, NULL);


            //  add the source fields 
            s = set->getSourceStructure();
            for (it = fields.begin(); it != fields.end(); ++it)
            {
                tango::IColumnInfoPtr col = s->createColumn();
                col->setName(towstr(it->source_name));
                col->setOffset(it->source_offset);
                col->setWidth(it->source_width);
                if (it->source_encoding)
                    col->setEncoding(it->source_encoding);
            }
            
            
            set->modifySourceStructure(s, NULL);


            // -- add the destination fields --
            s = set->getDestinationStructure();
            
            // for some reason, the delimited set creates a default structure if
            // there is none.  This may be right, but it screws up what we're trying
            // to do.  So, delete the default structure.

            col_count = s->getColumnCount();
            for (i = 0; i < col_count; ++i)
                s->deleteColumn(s->getColumnName(i));
            
            tango::IStructurePtr source_structure = set->getSourceStructure();
            int idx = 0;
            for (it = fields.begin(); it != fields.end(); ++it)
            {
                tango::IColumnInfoPtr col = s->createColumn();
                col->setName(towstr(it->name));
                col->setType(it->type);
                col->setWidth(it->width);
                col->setScale(it->scale);
                col->setExpression(towstr(it->formula));
                
                if (it->formula.length() == 0 && idx < source_structure->getColumnCount())
                    col->setExpression(source_structure->getColumnName(idx));
                ++idx;
            }
            
            set->modifyDestinationStructure(s, NULL);
            set->saveConfiguration();
            retval->setBoolean(true);
        }
    }
     else if (type == L"delimited")
    {
        std::wstring delimiters = L"";
        std::wstring line_delimiters = L"";
        std::wstring text_qualifiers = L"";
        bool first_row_column_names = true;
        std::vector<HostDataDefinitionField> fields;
        
        if (info->getMemberExists(L"delimiters"))
            delimiters = info->getMember(L"delimiters")->getString();
        if (info->getMemberExists(L"line_delimiters"))
            line_delimiters = info->getMember(L"line_delimiters")->getString();
        if (info->getMemberExists(L"text_qualifiers"))
            text_qualifiers = info->getMember(L"text_qualifiers")->getString();
        if (info->getMemberExists(L"first_row_column_names"))
            first_row_column_names = info->getMember(L"first_row_column_names")->getBoolean();
        
        if (info->getMemberExists(L"fields") && info->getMember(L"fields")->isObject())
        {
            kscript::ValueObject* fields_obj = info->getMember(L"fields")->getObject();
            size_t i, count = fields_obj->getRawMemberCount();
            for (i = 0; i < count; ++i)
            {
                kscript::Value* mval = fields_obj->getRawMemberByIdx(i);
                if (mval && mval->isObject())
                {
                    HostDataDefinitionField field;
                    field.name = L"";
                    field.type = 0;
                    field.width = 0;
                    field.scale = 0;
                    
                    int source_offset = 0, source_width = 0;
                    std::wstring name;
                    int type = 0, width = 0, scale = 0;

                    if (mval->getMemberExists(L"name"))
                        field.name = mval->getMember(L"name")->getString();
                    if (mval->getMemberExists(L"type"))
                        field.type = DbType::toTangoType(mval->getMember(L"type"));
                    if (mval->getMemberExists(L"width"))
                        field.width = mval->getMember(L"width")->getInteger();
                    if (mval->getMemberExists(L"scale"))
                        field.scale = mval->getMember(L"scale")->getInteger();
                    if (mval->getMemberExists(L"formula"))
                        field.formula = mval->getMember(L"formula")->getString();

                    fields.push_back(field);
                }
            }
        }
        
        std::wstring set_path;
        std::wstring filename = env->getParam(0)->getString();
        
        if (filename.substr(0,5) != L"file:")
        {
            if (xf_get_file_exist(filename))
                set_path = kl::filenameToUrl(filename);
                 else
                set_path = filename;
        }
         else
        {
            set_path = filename;
        }
        
        
        tango::IDelimitedTextSetPtr set;
        set = g_app->getDatabase()->openSetEx(set_path, tango::formatDelimitedText);
        if (set.isNull())
        {
            retval->setBoolean(false);
            return;
        }
        
        
        set->setDelimiters(delimiters, false);
        set->setLineDelimiters(line_delimiters, false);
        set->setTextQualifier(text_qualifiers, false);
        set->determineColumns(5000, NULL);
        set->setFirstRowColumnNames(first_row_column_names);
        
        tango::IStructurePtr source_structure = set->getSourceStructure();
        
        tango::IStructurePtr s;
        size_t i, col_count;
        std::vector<HostDataDefinitionField>::iterator it;


        // -- delete all existing destination fields --
        s = set->getDestinationStructure();
        col_count = s->getColumnCount();
        for (i = 0; i < col_count; ++i)
            s->deleteColumn(s->getColumnName(i));
        set->modifyDestinationStructure(s, NULL);


        


        // -- add the destination fields --
        s = set->getDestinationStructure();
        
        col_count = s->getColumnCount();
        for (i = 0; i < col_count; ++i)
            s->deleteColumn(s->getColumnName(i));

        int idx = 0;
        for (it = fields.begin(); it != fields.end(); ++it)
        {
            tango::IColumnInfoPtr col = s->createColumn();
            col->setName(towstr(it->name));
            col->setType(it->type);
            col->setWidth(it->width);
            col->setScale(it->scale);
            
            if (it->formula.length() == 0)
            {
                // no expression, simply take the corresponding source
                // field (based on index)
                col->setExpression(L"");
                
                if (idx < source_structure->getColumnCount())
                {
                    col->setExpression(source_structure->getColumnName(idx));
                }
            }
             else
            {
                std::wstring formula = towstr(it->formula);
                
                if (idx < source_structure->getColumnCount())
                {
                    std::wstring src_field_name = source_structure->getColumnName(idx);
                    formula = towstr(it->formula);
                    kl::replaceStrNoCase(formula, L"$SRCFIELD", src_field_name);
                }
                        
                col->setExpression(formula);
            }
            
            ++idx;
        }
        
        set->modifyDestinationStructure(s, NULL);
        set->saveConfiguration();
        
        retval->setBoolean(true);
        
    }
    
}


void HostData::loadDefinition(kscript::ExprEnv* env, kscript::Value* retval)
{
}


class HostImportExportFile
{
public:
    wxString source_path;
    wxString dest_path;
    bool compress;
};

static int extensionToDbtype(const wxString& filename)
{
    wxString ext;
    ext = filename.AfterLast(wxT('.'));
    ext.MakeLower();
    
    // specify what type of import we're doing based on the file extension
    if (ext.CmpNoCase(wxT("kpg")) == 0)
        return dbtypePackage;
     else if (ext.CmpNoCase(wxT("mdb")) == 0)
        return dbtypeAccess;
     else if (ext.CmpNoCase(wxT("accdb")) == 0)
        return dbtypeAccess;
     else if (ext.CmpNoCase(wxT("xls")) == 0)
        return dbtypeExcel;
     else if (ext.CmpNoCase(wxT("dbf")) == 0)
        return dbtypeXbase;

    return dbtypeDelimitedText;
}


static wxString appendPath(const wxString& str1, const wxString& str2)
{
    // determine target path for the imported table
    wxString dest_path = str1;
    dest_path.Trim(true);
    dest_path.Trim(false);

    if (dest_path.length() == 0 || dest_path[0] != '/')
        dest_path.Prepend(wxT("/"));
    if (dest_path.Last() != wxT('/'))
        dest_path += wxT("/");
    
    wxString temp = str2;
    temp.Trim(true);
    temp.Trim(false);
    if (temp.length() > 0 && temp.GetChar(0) == '/')
        temp.Remove(0, 1);
    dest_path += temp;
 
    return dest_path;
}

void HostData::importData(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
    
    if (env->getParamCount() == 1)
    {
        if (!env->getParam(0)->isObject())
            return;
        kscript::ValueObject* info = env->getParam(0)->getObject();
        
        int binding_database_type = -1;
        int database_type = -1;
        int port = -1;
        std::wstring server;
        std::wstring filename;
        std::wstring user_name;
        std::wstring password;
        std::wstring database;
        std::wstring target_path;
        
        // get the import parameters from the info object passed as a parameter
        if (info->getMemberExists(L"database_type"))
        {
            if (info->getMember(L"database_type")->isString())
                binding_database_type = DbDatabaseType::fromString(info->getMember(L"database_type")->getString());
                 else
                binding_database_type = info->getMember(L"database_type")->getInteger();
        }
        if (info->getMemberExists(L"port"))
            port = info->getMember(L"port")->getInteger();
        if (info->getMemberExists(L"filename"))                       // 'filename' is deprecated; replaced by 'path';
            filename = info->getMember(L"filename")->getString();     // here for backward compatibility
        if (info->getMemberExists(L"path"))
            filename = info->getMember(L"path")->getString();
        if (info->getMemberExists(L"server"))
            server = info->getMember(L"server")->getString();
        if (info->getMemberExists(L"user_name"))
            user_name = info->getMember(L"user_name")->getString();
        if (info->getMemberExists(L"password"))
            password = info->getMember(L"password")->getString();
        if (info->getMemberExists(L"database"))
            database = info->getMember(L"database")->getString();
        if (info->getMemberExists(L"target_path"))
            target_path = info->getMember(L"target_path")->getString();
        
        
        database_type = DbDatabaseType::toConnectionDatabaseType(binding_database_type);
        if (database_type == dbtypeUndefined)
            return;
    
        std::vector<HostImportExportFile> files;
        
        // old name was 'tables', but now it is 'objects'
        if (info->getMemberExists(L"objects") || info->getMemberExists(L"tables"))
        {
            kscript::Value* tables_val = NULL;
            
            if (info->getMemberExists(L"objects"))
                tables_val = info->getMember(L"objects");
            else if (info->getMemberExists(L"tables"))
                tables_val = info->getMember(L"tables");

            if (tables_val->isObject())
            {
                kscript::ValueObject* tables_obj = tables_val->getObject();
                size_t i, count = tables_obj->getRawMemberCount();
                for (i = 0; i < count; ++i)
                {
                    kscript::Value* mval = tables_obj->getRawMemberByIdx(i);
                    if (mval && mval->isObject())
                    {
                        std::wstring src, dest;
                        
                        // source_table and destination_table were the old names;
                        // these are included for backward compatibility
                        if (mval->getMemberExists(L"source_table"))
                            src = mval->getMember(L"source_table")->getString();
                        if (mval->getMemberExists(L"destination_table"))
                            dest = mval->getMember(L"destination_table")->getString();
                            
                        if (mval->getMemberExists(L"input"))
                            src = mval->getMember(L"input")->getString();
                        if (mval->getMemberExists(L"output"))
                            dest = mval->getMember(L"output")->getString();
                        if (src.length() > 0 && dest.length() > 0)
                        {
                            HostImportExportFile f;
                            f.source_path = src;
                            f.dest_path = dest;
                            files.push_back(f);
                        }
                    }
                }
            }
        }
        
        

        IJobPtr job;
        if (database_type == dbtypePackage)
        {
            ImportPkgJob* import_job = new ImportPkgJob;
            import_job->setPkgFilename(filename);
            
            std::vector<HostImportExportFile>::iterator it;
            for (it = files.begin(); it != files.end(); ++it)
            {
                import_job->addImportObject(it->source_path,
                                            appendPath(towx(target_path), it->dest_path));
            }
            
            job = static_cast<IJob*>(import_job);
        }
         else
        {
            ImportJob* import_job = new ImportJob;

            import_job->setImportType(database_type);
            
            if (filename.length() > 0)
            {
                import_job->setFilename(filename);
            }
             else
            {
                import_job->setConnectionInfo(server, port, database, user_name, password);
            }

            
            std::vector<HostImportExportFile>::iterator it;
            for (it = files.begin(); it != files.end(); ++it)
            {
                ImportJobInfo info;
                info.input_path = it->source_path;
                info.output_path = appendPath(towx(target_path), it->dest_path);
                info.append = false;
                
                import_job->addImportSet(info);
            }
            
            job = static_cast<IJob*>(import_job);
        }
        
        if (job.isNull())
        {
            retval->setBoolean(false);
            return;
        }
        
        job->runJob();
        job->runPostJob();

        retval->setBoolean(true);
    }
     else if (env->getParamCount() >= 2)
    {
        ImportJob* import_job = new ImportJob;
    
        
        ImportJobInfo info;
        info.input_path = env->getParam(0)->getString();
        info.output_path = env->getParam(1)->getString();
        info.append = false;
        info.specify_text_params = false;
        import_job->addImportSet(info);
        import_job->setImportType(extensionToDbtype(info.input_path));
        
        import_job->runJob();
        import_job->runPostJob();
        
        // need to make sure that the states are working before re-enabling this line 7/18/07
        //retval->setBoolean((export_job->getJobInfo()->getState() == jobStateFinished) ? true : false);
        retval->setBoolean(true);
        
                
        delete import_job;
    }
    
}

void HostData::exportData(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
    
    if (env->getParamCount() == 1)
    {
        if (!env->getParam(0)->isObject())
            return;
        kscript::ValueObject* info = env->getParam(0)->getObject();
        
        int binding_database_type = -1;
        int database_type = -1;
        int port = -1;
        std::wstring server;
        std::wstring filename;
        std::wstring user_name;
        std::wstring password;
        std::wstring database;
        bool raw = false;
        
        // get the import parameters from the info object passed as a parameter
        if (info->getMemberExists(L"database_type"))
        {
            if (info->getMember(L"database_type")->isString())
                binding_database_type = DbDatabaseType::fromString(info->getMember(L"database_type")->getString());
                 else
                binding_database_type = info->getMember(L"database_type")->getInteger();
        }
        if (info->getMemberExists(L"port"))
            port = info->getMember(L"port")->getInteger();
        if (info->getMemberExists(L"filename"))                       // 'filename' is deprecated; replaced by 'path';
            filename = info->getMember(L"filename")->getString();     // here for backward compatibility
        if (info->getMemberExists(L"path"))
            filename = info->getMember(L"path")->getString();
        if (info->getMemberExists(L"server"))
            server = info->getMember(L"server")->getString();
        if (info->getMemberExists(L"user_name"))
            user_name = info->getMember(L"user_name")->getString();
        if (info->getMemberExists(L"password"))
            password = info->getMember(L"password")->getString();
        if (info->getMemberExists(L"database"))
            database = info->getMember(L"database")->getString();
        if (info->getMemberExists(L"raw"))                            // raw mode for packages (default = false)
            raw = info->getMember(L"raw")->getBoolean();
        
        
                
        switch (binding_database_type)
        {
            // TODO: FIXME: there are missing types from this list, Xbase, etc.
            case DbDatabaseType::Xdnative:     database_type = dbtypeXdnative;     break;
            case DbDatabaseType::Package:      database_type = dbtypePackage;      break;
            case DbDatabaseType::Oracle:       database_type = dbtypeOracle;       break;
            case DbDatabaseType::SqlServer:    database_type = dbtypeSqlServer;    break;
            case DbDatabaseType::MySQL:        database_type = dbtypeMySql;        break;
            case DbDatabaseType::Odbc:         database_type = dbtypeOdbc;         break;
            case DbDatabaseType::Access:       database_type = dbtypeAccess;       break;
            case DbDatabaseType::Excel:        database_type = dbtypeExcel;        break;
            case DbDatabaseType::Filesystem:   database_type = dbtypeFilesystem;   break;
            default:
                return;
        }
    
        std::vector<HostImportExportFile> files;
        
        // old name was 'tables', but now it is 'objects'
        if (info->getMemberExists(L"objects") || info->getMemberExists(L"tables"))
        {
            kscript::Value* tables_val = NULL;
            
            if (info->getMemberExists(L"objects"))
                tables_val = info->getMember(L"objects");
            else if (info->getMemberExists(L"tables"))
                tables_val = info->getMember(L"tables");

            if (tables_val->isObject())
            {
                kscript::ValueObject* tables_obj = tables_val->getObject();
                size_t i, count = tables_obj->getRawMemberCount();
                for (i = 0; i < count; ++i)
                {
                    kscript::Value* mval = tables_obj->getRawMemberByIdx(i);
                    if (mval && mval->isObject())
                    {
                        std::wstring src, dest;
                        bool compress = true; // only used by package exports
                        
                        
                        // source_table and destination_table were the old names;
                        // these are included for backward compatibility
                        if (mval->getMemberExists(L"source_table"))
                            src = mval->getMember(L"source_table")->getString();
                        if (mval->getMemberExists(L"destination_table"))
                            dest = mval->getMember(L"destination_table")->getString();
                            
                        if (mval->getMemberExists(L"input"))
                            src = mval->getMember(L"input")->getString();
                        if (mval->getMemberExists(L"output"))
                            dest = mval->getMember(L"output")->getString();
                        if (mval->getMemberExists(L"compress"))
                            compress = mval->getMember(L"compress")->getBoolean();
                        if (src.length() > 0)
                        {
                            HostImportExportFile f;
                            f.source_path = src;
                            f.dest_path = dest;
                            f.compress = compress;
                            files.push_back(f);
                        }
                    }
                }
            }
        }
        
        IJobPtr job;
        if (database_type == dbtypePackage)
        {
            ExportPkgJob* export_job = new ExportPkgJob;
            export_job->setPkgFilename(filename, ExportPkgJob::modeOverwrite);
            export_job->setRawExport(raw);
            
            std::vector<HostImportExportFile>::iterator it;
            for (it = files.begin(); it != files.end(); ++it)
            {
                export_job->addExportObject(it->dest_path,
                                            it->source_path,
                                            it->compress);
            }
            
            job = static_cast<IJob*>(export_job);
        }
         else
        {
            ExportJob* export_job = new ExportJob;
            
            export_job->setExportType(database_type);
            
            if (filename.length() > 0)
            {
                export_job->setFilename(filename);
            }
             else
            {
                export_job->setConnectionInfo(server, port, database, user_name, password);
            }
            
            std::vector<HostImportExportFile>::iterator it;
            for (it = files.begin(); it != files.end(); ++it)
            {
                ExportJobInfo info;
                info.input_path = it->source_path;
                info.output_path = it->dest_path;
                info.append = false;
                
                export_job->addExportSet(info);
            }
            
            job = static_cast<IJob*>(export_job);
        }
        
        if (job.isNull())
        {
            retval->setBoolean(false);
            return;
        }
        
        job->runJob();
        //job->runPostJob(); // runPostJob() refreshes the tree, which we don't want

        retval->setBoolean(true);
    }
     else if (env->getParamCount() >= 2)
    {
        ExportJob* export_job = new ExportJob;
        
        
        ExportJobInfo info;
        info.input_path = env->getParam(0)->getString();
        info.output_path = env->getParam(1)->getString();
        info.append = false;
        export_job->setExportType(extensionToDbtype(info.output_path));

        export_job->addExportSet(info);
        
        export_job->runJob();
        //job->runPostJob(); // runPostJob() refreshes the tree, which we don't want
        
        // need to make sure that the states are working before re-enabling this line 7/18/07
        // retval->setBoolean((export_job->getJobInfo()->getState() == jobStateFinished) ? true : false);
        retval->setBoolean(true);
        
        delete export_job;
    }
    
} 


void HostData::importBinaryFile(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 2)
    {
        retval->setBoolean(false);
        return;
    }
    
    std::wstring input_path = env->getParam(0)->getString();
    std::wstring output_path = env->getParam(1)->getString();


    xf_file_t f = xf_open(input_path, xfOpen, xfRead, xfShareRead);
    if (!f)
    {
        retval->setBoolean(false);
        return;
    }
    
    
    std::wstring mime_type;;
    if (env->getParamCount() >= 3)
    {
        mime_type = env->getParam(2)->getString();
    }
     else
    {
        mime_type = determineMimeType(towx(input_path));
    }
    
    
    
    tango::IStreamPtr stream = g_app->getDatabase()->createStream(output_path, mime_type);
    if (!stream)
    {
        retval->setBoolean(false);
        return;
    }
    
    unsigned char* buf = new unsigned char[16400];
    while (1)
    {
        int read = xf_read(f, buf, 1, 16384);
        if (read == 0)
            break;
        buf[read] = 0;

        stream->write(buf, read, NULL);

        if (read != 16384)
            break;
    }
    delete[] buf;
    
    xf_close(f);
}

void HostData::exportBinaryFile(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 2)
    {
        retval->setBoolean(false);
        return;
    }
    
    std::wstring input_path = env->getParam(0)->getString();
    std::wstring output_path = env->getParam(1)->getString();


    tango::IStreamPtr src = g_app->getDatabase()->openStream(input_path);

    if (src.isNull())
    {
        retval->setBoolean(false);
        return;
    }

    xf_file_t f = xf_open(output_path, xfCreate, xfReadWrite, xfShareNone);
    if (!f)
    {
        retval->setBoolean(false);
        return;
    }


    unsigned char* buf = new unsigned char[32768];
    unsigned long read, written;
    
    while (1)
    {
        if (!src->read(buf, 32768, &read))
            break;
        if (read == 0)
            break;
          
        written = xf_write(f, buf, 1, read);
                
        if (written != read)
        {
            retval->setBoolean(false);
            return;
        }
        
        if (read != 32768)
            break;
    }

    xf_close(f);
}


void HostData::copyFile(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
    
    if (env->getParamCount() < 2)
        return;
        
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return;
        
    std::wstring param1 = env->getParam(0)->getString();
    std::wstring param2 = env->getParam(1)->getString();
    
    tango::IFileInfoPtr f = db->getFileInfo(param1);
    
    if (f.isNull())
        return;
        
    if (f->getType() == tango::filetypeSet)
    {
        tango::ISetPtr set = db->openSet(param1);
        if (set.isNull())
            return;
            
        // file is a table, we need a copy job
        CopyJob* job = new CopyJob;
        job->addCopyInstruction(db, set, wxT(""), wxT(""), wxT(""), db, param2);
        job->runJob();
        IJobInfoPtr info = job->getJobInfo();
        delete job;
        
        if (info->getState() == jobStateFinished)
            retval->setBoolean(true);
    }
     else
    {
        // a simply copy call to the db suffices
        retval->setBoolean(db->copyFile(param1, param2));
    }
    
} 

void HostData::readTextStream(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();
    
    if (env->getParamCount() < 1)
        return;
    
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return;
        
    std::wstring result;
    
    if (!readStreamTextFile(db, env->getParam(0)->getString(), result))
        return;
    
    retval->setString(result);
}


void HostData::writeTextStream(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
    
    if (env->getParamCount() < 2)
        return;
    
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return;
        
    std::wstring text = env->getParam(1)->getString();
    std::wstring mime_type;
    
    if (env->getParamCount() >= 3)
        mime_type = env->getParam(2)->getString();
         else
        mime_type = L"text/plain";
    
    if (!writeStreamTextFile(db, env->getParam(0)->getString(), text, mime_type))
        return;
    
    retval->setBoolean(true);
}



void HostData::groupQuery(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();
    
    if (env->getParamCount() < 6)
        return;
        
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return;
        
    std::wstring input = env->getParam(0)->getString();
    std::wstring output = env->getParam(1)->getString();
    std::wstring group = env->getParam(2)->getString();
    std::wstring output_fields = env->getParam(3)->getString();
    std::wstring where_condition = env->getParam(4)->getString();
    std::wstring having_condition = env->getParam(5)->getString();
    

    
    tango::ISetPtr input_set = db->openSet(input);
    if (input_set.isNull())
        return;
    
    tango::ISetPtr output_set;
    output_set = db->runGroupQuery(input_set,
                                   group,
                                   output_fields,
                                   where_condition,
                                   having_condition,
                                   NULL);
    
    if (output_set.isNull())
        return;
    
    if (output.length() > 0)
    {
        db->storeObject(output_set.p, output);
    }
    
    tango::IIteratorPtr iter = output_set->createIterator(L"", L"", NULL);
    if (iter.isNull())
        return;
        
    DbResult* res = DbResult::createObject(env);
    res->init(iter);
    
    retval->setObject(res);
}




HostPrinting::HostPrinting()
{
}

HostPrinting::~HostPrinting()
{
}


void HostPrinting::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

void HostPrinting::saveAsPdf(kscript::ExprEnv* env, kscript::Value* retval)
{
    // note: the first parameter is the template to use when creating 
    // the pdf or an array of templates, all of which will be merged
    // to create the pdf; the second parameter is the path to which 
    // to save the pdf
    int param_count = env->getParamCount();
    if (param_count < 2)
    {
        retval->setBoolean(false);
        return;
    }

    // get the input template(s) from the first parameter
    std::vector<wxString> inputs;

    if (env->getParam(0)->isObject() &&
        env->getParam(0)->getObject()->isKindOf(L"Array"))
    {
        // input is an array; add on each of the elements
        // of the array
        kscript::ValueObject* obj = env->getParam(0)->getObject();
        
        size_t i, cnt = obj->getRawMemberCount();
        for (i = 0; i < cnt; ++i)
        {
            std::wstring member_name = obj->getRawMemberName(i);
            kscript::Value* member_obj = obj->getRawMemberByIdx(i);
            inputs.push_back(towx(member_obj->getString()));
        }
    }
     else
    {
        // treat input as a string
        inputs.push_back(env->getParam(0)->getString());
    }

    // if we don't have anything to work with, we're done
    if (inputs.empty())
    {
        retval->setBoolean(false);
        return;
    }

    // create an engine for each input template; note: the last parameter
    // is the output path, so iterate over all input elements except the
    // last one
    std::vector<ReportLayoutEngine> engines;
    
    std::vector<wxString>::iterator it, it_end;
    it_end = inputs.end();
    
    for (it = inputs.begin(); it != it_end; ++it)
    {
        wxString input_path = *it;

        kcanvas::ICanvasPtr canvas = kcanvas::Canvas::create();
        kcanvas::IComponentPtr report_design = CompReportDesign::create();
        
        CompReportDesign* report_design_ptr = static_cast<CompReportDesign*>(report_design.p);
        report_design_ptr->setCanvas(canvas);

        // load the input; if we can't load one of the inputs,
        // we're done
        if (!report_design_ptr->load(input_path))
        {
            retval->setBoolean(false);
            return;
        }

        report_design_ptr->layout();

        // create an engine for this particular input item
        ReportLayoutEngine layout_engine;        
        
        int page_width, page_height;
        report_design_ptr->getPageSize(&page_width, &page_height);

        int margin_left, margin_right, margin_top, margin_bottom;
        report_design_ptr->getPageMargins(&margin_left, &margin_right, &margin_top, &margin_bottom);

        double width = ((double)page_width)/kcanvas::CANVAS_MODEL_DPI;
        double height = ((double)page_height)/kcanvas::CANVAS_MODEL_DPI;

        std::vector<ReportSection> sections;
        report_design_ptr->getSections(sections);
        
        wxString data_source = report_design_ptr->getDataSource();
        wxString data_filter = report_design_ptr->getDataFilter();

        layout_engine.init(sections,
                           data_source,
                           data_filter,
                           page_width,
                           page_height,
                           margin_left,
                           margin_right,
                           margin_top,
                           margin_bottom);

        engines.push_back(layout_engine);
    }

    // set the page range
    ReportPrintInfo info;
    info.setQuality(600);
    info.setMinPage(1);
    info.setMaxPage(1);
    info.setFromPage(1);
    info.setToPage(1);
    info.setAllPages(true);

    // get the output path, which is the second parameter
    wxString output_path = env->getParam(1)->getString();

    // create a new report output pdf; it will destroy 
    // itself when done; block until creation is complete
    ReportOutputPdf* pdf = new ReportOutputPdf(engines, info, output_path, true);
    pdf->create();

    retval->setBoolean(true);
}









HostAutomation::HostAutomation()
{
}

HostAutomation::~HostAutomation()
{
}

void HostAutomation::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

void HostAutomation::command(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
    
    if (env->getParamCount() < 1)
        return;
        
    int id = CommandMgr::lookupCommandId(env->getParam(0)->getString());
    if (id < 0)
        return;
    
    // post an event to ourselves
    wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED, id);
    ::wxPostEvent(g_app->getMainWindow()->GetEventHandler(), e);
}

void HostAutomation::execute(kscript::ExprEnv* env, kscript::Value* retval)
{
}


kscript::Value* HostAutomation::getMember(const std::wstring& name)
{
    if (name == L"activeDocument")
    {
        m_retval.setNull();
        
        IFramePtr frame = g_app->getMainFrame();
        if (frame.isNull())
            return &m_retval;
            
        IDocumentSitePtr site = frame->getActiveChild();
        if (site.isNull())
            return &m_retval;
        
        HostDocument* doc = HostDocument::createObject(getParser());
        doc->m_site = site;
        m_retval.setObject(doc);
        
        return &m_retval;
    }
   
    return BaseClass::getMember(name);
}

void HostAutomation::waitForRunningJobs(kscript::ExprEnv* env, kscript::Value* retval)
{
    int wait_ms = 10;
    IJobQueuePtr job_queue = g_app->getJobQueue();
    while (job_queue->getJobsActive())
    {
        kl::Thread::sleep(wait_ms);
        if (wait_ms < 1000)
            wait_ms++;
    }
}

