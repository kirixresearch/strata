/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-01-28
 *
 */


#ifndef __APP_SCRIPTHOSTAPP_H
#define __APP_SCRIPTHOSTAPP_H


#include "scriptapp.h"


class Console : public kscript::ValueObject
{
    BEGIN_KSCRIPT_CLASS("Console", Console)
        KSCRIPT_METHOD("constructor", Console::constructor)
        KSCRIPT_STATIC_GUI_METHOD("show", Console::show)
        KSCRIPT_STATIC_GUI_METHOD("write", Console::write)
        KSCRIPT_STATIC_GUI_METHOD("writeLine", Console::writeLine)
        KSCRIPT_STATIC_GUI_METHOD("clear", Console::clear)
    END_KSCRIPT_CLASS()

public:

    Console();
    ~Console();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    static void show(kscript::ExprEnv* env, void*, kscript::Value* retval);
    static void write(kscript::ExprEnv* env, void*, kscript::Value* retval);
    static void writeLine(kscript::ExprEnv* env, void*, kscript::Value* retval);
    static void clear(kscript::ExprEnv* env, void*, kscript::Value* retval);

private:

    ApplicationWeakReference m_app;
};


// -- Extension utility functions --

class Extension : public kscript::ValueObject
{
    BEGIN_KSCRIPT_CLASS("Extension", Extension)
        KSCRIPT_METHOD("constructor", Extension::constructor)
        KSCRIPT_STATIC_METHOD("getBitmapResource", Extension::getBitmapResource)
        KSCRIPT_STATIC_METHOD("getTextResource", Extension::getTextResource)
        KSCRIPT_STATIC_METHOD("isContextPackage", Extension::isContextPackage)
    END_KSCRIPT_CLASS()

public:

    Extension();
    ~Extension();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    static void getBitmapResource(kscript::ExprEnv* env, void*, kscript::Value* retval);
    static void getTextResource(kscript::ExprEnv* env, void*, kscript::Value* retval);
    static void isContextPackage(kscript::ExprEnv* env, void*, kscript::Value* retval);
};


// -- Application Host bindings --

class HostApp : public kscript::ValueObject,
                public xcm::signal_sink
{
public:
    enum
    {
        DockTop = 1,
        DockLeft = 2,
        DockRight = 3,
        DockBottom = 4,
        DockFloating = 5
    };
    
    enum
    {
        ExecuteWait = 1 << 0,
        ExecuteSource = 1 << 1,
        ExecuteTemplate = 1 << 2
    };
    
    
    BEGIN_KSCRIPT_CLASS("HostApp", HostApp)
        KSCRIPT_METHOD("constructor", HostApp::constructor)
        KSCRIPT_GUI_METHOD("getDatabase", HostApp::getDatabase)
        KSCRIPT_GUI_METHOD("openDatabase", HostApp::openDatabase)
        KSCRIPT_GUI_METHOD("createDatabase", HostApp::createDatabase)
        KSCRIPT_GUI_METHOD("getCurrentLocation", HostApp::getCurrentLocation)
        KSCRIPT_GUI_METHOD("setFrameCaption", HostApp::setFrameCaption)
        KSCRIPT_GUI_METHOD("getFrameCaption", HostApp::getFrameCaption)
        KSCRIPT_GUI_METHOD("getFrameMenu", HostApp::getFrameMenu)          
        KSCRIPT_GUI_METHOD("showFrame", HostApp::showFrame)
        KSCRIPT_GUI_METHOD("newDocument", HostApp::newDocument)
        KSCRIPT_GUI_METHOD("open", HostApp::open)
        KSCRIPT_GUI_METHOD("openWeb", HostApp::openWeb)
        KSCRIPT_GUI_METHOD("sendWebRequest", HostApp::sendWebRequest);
        KSCRIPT_GUI_METHOD("close", HostApp::close);
        KSCRIPT_GUI_METHOD("getDocuments", HostApp::getDocuments);
        KSCRIPT_GUI_METHOD("getDocumentById", HostApp::getDocumentById);
        KSCRIPT_GUI_METHOD("getActiveDocument", HostApp::getActiveDocument);
        KSCRIPT_GUI_METHOD("createPane", HostApp::createPane);
        KSCRIPT_GUI_METHOD("createDocument", HostApp::createDocument);
        KSCRIPT_GUI_METHOD("showPane", HostApp::showPane);
        KSCRIPT_GUI_METHOD("hang", HostApp::hang)
        KSCRIPT_GUI_METHOD("crash", HostApp::crash)
        KSCRIPT_GUI_METHOD("exit", HostApp::exit)
        KSCRIPT_GUI_METHOD("refresh", HostApp::refresh)
        KSCRIPT_METHOD("execute", HostApp::execute)
        KSCRIPT_METHOD("executeAndWait", HostApp::executeAndWait)
        KSCRIPT_METHOD("checkVersion", HostApp::checkVersion)
        KSCRIPT_METHOD("getVersion", HostApp::getVersion)
        KSCRIPT_METHOD("getApplicationId", HostApp::getApplicationId)
        KSCRIPT_METHOD("getCommandLineArguments", HostApp::getCommandLineArguments)
        KSCRIPT_METHOD("isGuiActive", HostApp::isGuiActive)
        KSCRIPT_METHOD("isService", HostApp::isService)

        // -- to be deprecated --
        KSCRIPT_GUI_METHOD("showJobManager", HostApp::showJobManager)         
        
        // -- deprecated --  
        KSCRIPT_GUI_METHOD("createJob", HostApp::createJob)

        // -- private test function --
        KSCRIPT_METHOD("evalExpression", HostApp::evalExpression)

    END_KSCRIPT_CLASS()

public:

    HostApp();
    ~HostApp();

    void init(kscript::ExprParser* expr, Application* app);

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void getDatabase(kscript::ExprEnv* env, kscript::Value* retval);
    void openDatabase(kscript::ExprEnv* env, kscript::Value* retval);
    void createDatabase(kscript::ExprEnv* env, kscript::Value* retval);
    void getCurrentLocation(kscript::ExprEnv* env, kscript::Value* retval);
    void setFrameCaption(kscript::ExprEnv* env, kscript::Value* retval);
    void getFrameCaption(kscript::ExprEnv* env, kscript::Value* retval);
    void getFrameMenu(kscript::ExprEnv* env, kscript::Value* retval);
    void showFrame(kscript::ExprEnv* env, kscript::Value* retval);
    void showJobManager(kscript::ExprEnv* env, kscript::Value* retval);
    void createJob(kscript::ExprEnv* env, kscript::Value* retval);
    void newDocument(kscript::ExprEnv* env, kscript::Value* retval);
    void open(kscript::ExprEnv* env, kscript::Value* retval);
    void openWeb(kscript::ExprEnv* env, kscript::Value* retval);
    void sendWebRequest(kscript::ExprEnv* env, kscript::Value* retval);
    void close(kscript::ExprEnv* env, kscript::Value* retval);
    void getDocuments(kscript::ExprEnv* env, kscript::Value* retval);
    void getDocumentById(kscript::ExprEnv* env, kscript::Value* retval);
    void getActiveDocument(kscript::ExprEnv* env, kscript::Value* retval);
    void createDocument(kscript::ExprEnv* env, kscript::Value* retval);
    void createPane(kscript::ExprEnv* env, kscript::Value* retval);
    void showPane(kscript::ExprEnv* env, kscript::Value* retval);
    void hang(kscript::ExprEnv* env, kscript::Value* retval);
    void crash(kscript::ExprEnv* env, kscript::Value* retval);
    void exit(kscript::ExprEnv* env, kscript::Value* retval);
    void refresh(kscript::ExprEnv* env, kscript::Value* retval);
    void execute(kscript::ExprEnv* env, kscript::Value* retval);
    void getCommandLineArguments(kscript::ExprEnv* env, kscript::Value* retval);
    void checkVersion(kscript::ExprEnv* env, kscript::Value* retval);
    void getVersion(kscript::ExprEnv* env, kscript::Value* retval);
    void getApplicationId(kscript::ExprEnv* env, kscript::Value* retval);
    void executeAndWait(kscript::ExprEnv* env, kscript::Value* retval);
    void evalExpression(kscript::ExprEnv* env, kscript::Value* retval);
    void isGuiActive(kscript::ExprEnv* env, kscript::Value* retval);
    void isService(kscript::ExprEnv* env, kscript::Value* retval);

public:

    Application* getApp();
    size_t invokeJsEvent(const std::wstring& evt,
                         kscript::Value* event_args = NULL,
                         unsigned int event_flags = 0);

private:

    // signal handlers for host app events
    void onActiveChildChanged(IDocumentSitePtr doc_site);
    void onFrameEvent(FrameworkEvent& evt);
    void onFrameDestroy();

private:

    ApplicationWeakReference m_app;
    IFramePtr m_frame;
    wxString m_location;
};


class HostDocument : public kscript::ValueObject
{
    BEGIN_KSCRIPT_CLASS("HostDocument", HostDocument)
        KSCRIPT_GUI_METHOD("constructor", HostDocument::setCaption)
        KSCRIPT_GUI_METHOD("getCaption", HostDocument::getCaption)
        KSCRIPT_GUI_METHOD("getLocation", HostDocument::getLocation)
        KSCRIPT_GUI_METHOD("getType", HostDocument::getType)
        KSCRIPT_GUI_METHOD("getDOMDocument", HostDocument::getDOMDocument)
    END_KSCRIPT_CLASS()

public:

    void setCaption(kscript::ExprEnv* env, kscript::Value* retval);
    void getCaption(kscript::ExprEnv* env, kscript::Value* retval);
    void getLocation(kscript::ExprEnv* env, kscript::Value* retval);
    void getType(kscript::ExprEnv* env, kscript::Value* retval);
    void getId(kscript::ExprEnv* env, kscript::Value* retval);
    
    // TODO: need to document
    void getDOMDocument(kscript::ExprEnv* env, kscript::Value* retval);

    kscript::Value* getMember(const std::wstring& name);

public:

    IDocumentSitePtr m_site;
};


class HostGlobal : public kscript::ValueObject
{
    BEGIN_KSCRIPT_CLASS("HostGlobal", HostGlobal)
        KSCRIPT_METHOD("constructor", HostGlobal::constructor)
    END_KSCRIPT_CLASS()

public:

    HostGlobal();
    ~HostGlobal();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    
    void setMember(const std::wstring& name, kscript::Value* value);
    kscript::Value* getMember(const std::wstring& name);
};


class HostJob : public kscript::ValueObject
{
    BEGIN_KSCRIPT_CLASS("HostJob", HostJob)
        KSCRIPT_METHOD("constructor", HostJob::constructor)
        KSCRIPT_GUI_METHOD("cancel", HostJob::cancel)
        KSCRIPT_GUI_METHOD("finish", HostJob::finish)
        KSCRIPT_METHOD("getMaximum", HostJob::getMaximum)
        KSCRIPT_METHOD("getValue", HostJob::getValue)
        KSCRIPT_METHOD("incrementValue", HostJob::incrementValue)
        KSCRIPT_METHOD("isCancelling", HostJob::isCancelling)
        KSCRIPT_GUI_METHOD("start", HostJob::start)
        KSCRIPT_METHOD("setMaximum", HostJob::setMaximum)
        KSCRIPT_METHOD("setTitle", HostJob::setTitle)
        KSCRIPT_METHOD("setValue", HostJob::setValue)
    END_KSCRIPT_CLASS()

public:

    HostJob();
    ~HostJob();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void cancel(kscript::ExprEnv* env, kscript::Value* retval);
    void finish(kscript::ExprEnv* env, kscript::Value* retval);
    void getMaximum(kscript::ExprEnv* env, kscript::Value* retval);
    void getValue(kscript::ExprEnv* env, kscript::Value* retval);
    void incrementValue(kscript::ExprEnv* env, kscript::Value* retval);
    void isCancelling(kscript::ExprEnv* env, kscript::Value* retval);
    void start(kscript::ExprEnv* env, kscript::Value* retval);
    void setMaximum(kscript::ExprEnv* env, kscript::Value* retval);
    void setTitle(kscript::ExprEnv* env, kscript::Value* retval);
    void setValue(kscript::ExprEnv* env, kscript::Value* retval);
    
private:

    IJobInfoPtr m_job_info;
};


class HostServices : public kscript::ValueObject
{
    BEGIN_KSCRIPT_CLASS("HostServices", HostServices)
        KSCRIPT_METHOD("constructor", HostServices::constructor)
        KSCRIPT_METHOD("get", HostServices::get)
    END_KSCRIPT_CLASS()

public:

    HostServices();
    ~HostServices();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void get(kscript::ExprEnv* env, kscript::Value* retval);
    
    kscript::Value* getMember(const std::wstring& name);

private:

    void getServiceInternal(const std::wstring service_name, kscript::Value* retval);
        
private:

    std::map<std::wstring, kscript::ValueObject*> m_services;
};


class HostBitmap : public kscript::ValueObject
{
    BEGIN_KSCRIPT_CLASS("HostBitmap", HostBitmap)
        KSCRIPT_STATIC_METHOD("get", HostBitmap::get)
    END_KSCRIPT_CLASS()

public:

    HostBitmap();
    ~HostBitmap();

    static void get(kscript::ExprEnv* env, void*, kscript::Value* retval);

private:

    static void getBitmapInternal(kscript::ExprEnv* env,
                                  const std::wstring bitmap_name,
                                  const int bitmap_size,
                                  kscript::Value* retval);
};


class HostPreferences : public kscript::ValueObject
{
    BEGIN_KSCRIPT_CLASS("HostPreferences", HostPreferences)
        KSCRIPT_METHOD("constructor", HostPreferences::constructor)
        KSCRIPT_GUI_METHOD("setValue", HostPreferences::setValue)
        KSCRIPT_GUI_METHOD("getValue", HostPreferences::getValue)
        KSCRIPT_GUI_METHOD("getAll", HostPreferences::getAll)
        KSCRIPT_GUI_METHOD("deleteValue", HostPreferences::deleteValue)
        KSCRIPT_GUI_METHOD("exists", HostPreferences::exists)
    END_KSCRIPT_CLASS()

public:

    HostPreferences();
    ~HostPreferences();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setValue(kscript::ExprEnv* env, kscript::Value* retval);
    void getValue(kscript::ExprEnv* env, kscript::Value* retval);
    void deleteValue(kscript::ExprEnv* env, kscript::Value* retval);
    void exists(kscript::ExprEnv* env, kscript::Value* retval);
    void getAll(kscript::ExprEnv* env, kscript::Value* retval);
};


class HostData : public kscript::ValueObject
{
    BEGIN_KSCRIPT_CLASS("HostData", HostData)
        KSCRIPT_METHOD("constructor", HostData::constructor)
        KSCRIPT_METHOD("assignDefinition", HostData::assignDefinition)
        KSCRIPT_METHOD("loadDefinition", HostData::loadDefinition)
        KSCRIPT_METHOD("importData", HostData::importData)
        KSCRIPT_METHOD("exportData", HostData::exportData)
        KSCRIPT_METHOD("importBinaryFile", HostData::importBinaryFile)
        KSCRIPT_METHOD("exportBinaryFile", HostData::exportBinaryFile)
        KSCRIPT_METHOD("copyFile", HostData::copyFile)
        KSCRIPT_METHOD("readTextStream", HostData::readTextStream)
        KSCRIPT_METHOD("writeTextStream", HostData::writeTextStream)
    END_KSCRIPT_CLASS()

public:

    HostData();
    ~HostData();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void assignDefinition(kscript::ExprEnv* env, kscript::Value* retval);
    void loadDefinition(kscript::ExprEnv* env, kscript::Value* retval);
    void importData(kscript::ExprEnv* env, kscript::Value* retval);
    void exportData(kscript::ExprEnv* env, kscript::Value* retval);
    void importBinaryFile(kscript::ExprEnv* env, kscript::Value* retval);
    void exportBinaryFile(kscript::ExprEnv* env, kscript::Value* retval);
    void copyFile(kscript::ExprEnv* env, kscript::Value* retval);
    void readTextStream(kscript::ExprEnv* env, kscript::Value* retval);
    void writeTextStream(kscript::ExprEnv* env, kscript::Value* retval);
    void groupQuery(kscript::ExprEnv* env, kscript::Value* retval);
};


class HostPrinting : public kscript::ValueObject
{
    BEGIN_KSCRIPT_CLASS("HostPrinting", HostPrinting)
        KSCRIPT_METHOD("constructor", HostPrinting::constructor)
        KSCRIPT_METHOD("saveAsPdf", HostPrinting::saveAsPdf)
    END_KSCRIPT_CLASS()

public:

    HostPrinting();
    ~HostPrinting();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void saveAsPdf(kscript::ExprEnv* env, kscript::Value* retval);
};


class HostAutomation : public kscript::ValueObject
{
    typedef kscript::ValueObject BaseClass;

    BEGIN_KSCRIPT_CLASS("HostAutomation", HostAutomation)
        KSCRIPT_METHOD("constructor", HostAutomation::constructor)
        KSCRIPT_GUI_METHOD("command", HostAutomation::command)
        KSCRIPT_GUI_METHOD("execute", HostAutomation::execute)
        KSCRIPT_METHOD("waitForRunningJobs", HostAutomation::waitForRunningJobs)
    END_KSCRIPT_CLASS()

public:

    HostAutomation();
    ~HostAutomation();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void command(kscript::ExprEnv* env, kscript::Value* retval);
    void execute(kscript::ExprEnv* env, kscript::Value* retval);
    void waitForRunningJobs(kscript::ExprEnv* env, kscript::Value* retval);
    
    kscript::Value* getMember(const std::wstring& name);
    
public:

    kscript::Value m_retval;
};


#endif

