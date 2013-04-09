/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-07-29
 *
 */


#include "appmain.h"
#include "scripthost.h"
#include "scriptdb.h"
#include "scriptdlg.h"
#include "scriptgraphics.h"
#include "scriptbitmap.h"
#include "scriptgui.h"
#include "scriptapp.h"
#include "scriptmenu.h"
#include "scripttext.h"
#include "scriptcombo.h"
#include "scriptbutton.h"
#include "scripttoolbar.h"
#include "scripttreeview.h"
#include "scriptlayout.h"
#include "scriptlist.h"
#include "scriptlistview.h"
#include "scriptmemory.h"
#include "scriptfile.h"
#include "scriptfiletransfer.h"
#include "scripthttp.h"
#include "scriptslider.h"
#include "scriptspin.h"
#include "scriptstatusbar.h"
#include "scriptwebbrowser.h"
#include "scriptwebdom.h"
#include "scriptxml.h"
#include "scriptenvironment.h"
#include "scriptprogress.h"
#include "scriptbanner.h"
#include "scriptfont.h"
#include "scripthostapp.h"
#include "scriptsystem.h"
#include "scriptnative.h"
#include "scriptbase64.h"
#include "scriptcrypt.h"
#include "scripthash.h"
#include "scriptreport.h"
#include "scripttable.h"
#include "scriptprocess.h"
#include "appcontroller.h"
#include "panelconsole.h"
#include "dlgdatabasefile.h"
#include "jobscript.h"
#include "extensionmgr.h"
#include "extensionpkg.h"


#include <wx/url.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <kl/utf8.h>
#include <kl/thread.h>


enum
{
    ID_GuiMarshal = wxID_HIGHEST + 1,
    ID_MessageBox,
    ID_TextEntry,
    ID_Browse,
    ID_ProjectFileDialog,
    ID_FileDialog
};



// -- the gui marshaler --

GuiMarshal gui_marshal;



// -- fast wxCommandEvent - basically, doesn't clone --

class GuiMarshalEvent : public wxCommandEvent
{
public:
    GuiMarshalEvent(wxMutex& _mutex,
                    wxCondition& _condition)
                    : wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, ID_GuiMarshal),
                    mutex(_mutex),
                    condition(_condition)
    {
    }
    
    wxEvent* Clone() const { return (wxEvent*)this; }    
    

    // parameters
    wxMutex& mutex;
    wxCondition& condition;

    kscript::ExprEnv* env;
    kscript::Value* retval;
    kscript::ExprBindFunc func;
    void* param;
};


// -- GUI Marshaling machinery --

GuiMarshal::GuiMarshal()
{
}

GuiMarshal::~GuiMarshal()
{
    std::vector<GuiMarshalFunc*>::iterator it;
    for (it = m_to_delete.begin(); it != m_to_delete.end(); ++it)
        delete (*it);
    
    std::vector<kscript::BoundMemberInfoBase*>::iterator it2;
    for (it2 = m_to_delete2.begin(); it2 != m_to_delete2.end(); ++it2)
        delete (*it2);
}

void GuiMarshal::func_guiMarshal(kscript::ExprEnv* env,
                                 void* param,
                                 kscript::Value* retval)
{
    GuiMarshalFunc* p = (GuiMarshalFunc*)param;

    if (kl::Thread::isMain())
    {
        // if we are in the main thread, no marshaling is necessary
        p->func(env, p->param, retval);
        return;
    }
    
    ScriptHost* sh = (ScriptHost*)env->getParser()->getExtraLong();
    GuiMarshalSync* g = sh->pushGuiMarshalSync();
    g->mutex->Lock();
    
    // -- fire an event to ourselves --
    GuiMarshalEvent* e = new GuiMarshalEvent(*(g->mutex), *(g->condition));
    e->env = env;
    e->retval = retval;
    e->param = p->param;
    e->func = p->func;
    gui_marshal.AddPendingEvent(*e);

    g->condition->Wait();
    g->mutex->Unlock();

    sh->popGuiMarshalSync();
}

bool GuiMarshal::ProcessEvent(wxEvent& evt)
{
    GuiMarshalEvent* p = &((GuiMarshalEvent&)evt);
    p->func(p->env, p->param, p->retval);

    p->mutex.Lock();
    p->condition.Signal();
    p->mutex.Unlock();

    return true;
}

void GuiMarshal::addGuiFunction(kscript::ExprParser* expr,
                                const std::wstring& func_name,
                                kscript::ExprBindFunc func_ptr,
                                const std::wstring& formats,
                                void* param)
{
    GuiMarshalFunc* p = new GuiMarshalFunc;
    p->func = func_ptr;
    p->param = param;

    m_to_delete.push_back(p);
    
    expr->addFunction(func_name,
                      true,
                      func_guiMarshal,
                      false,
                      formats,
                      p);
}

void GuiMarshal::getRerouteInfo(kscript::ExprBindFunc func_ptr,
                                void* param,
                                kscript::ExprBindFunc* result_ptr,
                                void** result_param)
{
    GuiMarshalFunc* p = new GuiMarshalFunc;
    p->func = func_ptr;
    p->param = param;
    
    m_to_delete.push_back(p);
    
    *result_ptr = func_guiMarshal;
    *result_param = (void*)p;
}


void GuiMarshal::getRerouteInfo(kscript::ExprBindFunc func_ptr,
                                void* param,
                                kscript::Value* retval)
{
    kscript::ExprBindFunc f;
    void* p;
    
    getRerouteInfo(func_ptr, param, &f, &p);
    retval->setFunction(f, p);
}



// -- ApplicationWeakReference class implementation --

ApplicationWeakReference::ApplicationWeakReference()
{
    m_app = NULL;
}

void ApplicationWeakReference::setApp(Application* app)
{
    m_app = app;
    app->sigDestructing.connect(this, &ApplicationWeakReference::onAppDestructing);
}

Application* ApplicationWeakReference::getApp()
{
    return m_app;
}

void ApplicationWeakReference::onAppDestructing()
{
    m_app = NULL;
}
  




// -- ScriptHostBase class implementation --

// you can derive your class from kscript::ValueObject if you want;
// deriving from this intermediate class provides a way of getting
// the current Application* object as well as the current ScriptHost* object;
// In order for this to work, you have to call initComponent() from
// your object's MyObject::constructor method

ScriptHostBase::ScriptHostBase()
{
    m_script_host = NULL;
}

Application* ScriptHostBase::getApp()
{
    Application* ret = m_app.getApp();
    
    // note if this is returning NULL, you probably forgot to call
    // FormComponent::initComponent() from your component's constructor
    
    return ret;
}

ScriptHost* ScriptHostBase::getScriptHost()
{
    // note if this is returning NULL, you probably forgot to call
    // FormComponent::initComponent() from your component's constructor

    return m_script_host;
}


void ScriptHostBase::initComponent(kscript::ExprEnv* env)
{
    if (m_app.getApp())
        return;
        
    kscript::ExprParser* parser = env->getParser();
    wxASSERT_MSG(parser != NULL, wxT("parser != NULL"));
    
    m_script_host = (ScriptHost*)parser->getExtraLong();
    wxASSERT_MSG(m_script_host, wxT("Script host ptr is null"));
    
    kscript::Value* val = parser->getBindVariable(L"Application");
    wxASSERT_MSG(val != NULL, wxT("val != NULL"));
    
    Application* app = (Application*)val->getObject();
    wxASSERT_MSG(app != NULL, wxT("app != NULL"));
    
    m_app.setApp(app);
}

size_t ScriptHostBase::getJsEventSinkCount(const std::wstring& evt)
{
    kscript::Value* vevt = getMember(evt);
    if (!vevt)
        return 0;

    Event* e = (Event*)vevt->getObject();
    if (!e)
        return 0;
        
    return e->getSinkCountInternal();
}

size_t ScriptHostBase::invokeJsEvent(const std::wstring& evt,
                                     kscript::Value* event_args,
                                     unsigned int event_flags)
{
    kscript::Value* vevt = getMember(evt);
    if (!vevt)
        return 0;

    return invokeJsEvent(vevt, event_args, event_flags);
}

size_t ScriptHostBase::invokeJsEvent(kscript::Value* vevt,
                                     kscript::Value* event_args,
                                     unsigned int event_flags)
{
    if (!vevt->isObject())
        return 0;
        
    kscript::ValueObject* vobj = vevt->getObject();
    if (!vobj->isKindOf(Event::staticGetClassId()))
        return 0;
        
    Event* e = (Event*)vobj;
    
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


void func_print(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    wxString message = env->m_eval_params[0]->getString();
    message += "\n";
    
    ScriptHost* script_host = (ScriptHost*)env->getParser()->getExtraLong();
    if (script_host->m_print_function.isFunction())
    {
        kscript::Function* f = script_host->m_print_function.getFunction();
        
        kscript::Value* params[1];
        params[0] = env->getParam(0);
        env->getParser()->invoke(env, &script_host->m_print_function, env->getThis(), retval, params, 1);
    }
     else
    {
        g_app->getAppController()->printConsoleText(message);
    }
}

void func_exit(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    env->setRuntimeError(kscript::rterrorExit);
}

void func_alert(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    wxString message, caption;
    int msg_style;


    message = env->m_eval_params[0]->getString();


    if (env->m_param_count > 1)
        caption = env->m_eval_params[1]->getString();
         else
        caption = APPLICATION_NAME;

    msg_style = wxOK;

    if (env->m_param_count > 2)
    {
        int style = env->m_eval_params[2]->getInteger();

        msg_style = 0;

        if (style & DialogResult::Ok)
            msg_style |= wxOK;
        
        if (style & DialogResult::YesNo)
            msg_style |= wxYES_NO;

        if (style & DialogResult::Cancel)
            msg_style |= wxCANCEL;
    }

    // -- fire an event to ourselves --

    int res = appMessageBox(message, caption, msg_style);
    
    switch (res)
    {
        case wxYES:
            res = DialogResult::Yes;
            break;

        case wxNO:
            res = DialogResult::No;
            break;

        case wxCANCEL:
            res = DialogResult::Cancel;
            break;

        default:
        case wxOK:
            res = DialogResult::Ok;
            break;
    }

    retval->setInteger(res);
}


// -- ScriptHost implementation --

BEGIN_EVENT_TABLE(ScriptHost, wxEvtHandler)
END_EVENT_TABLE()

ScriptHost::ScriptHost()
{
    m_error_code = 0;
    m_error_line = 0;
    m_error_offset = 0;
    m_error_string = wxT("");

    m_pkg = NULL;
    
    m_gui_sync_pos = 0;
    
    m_expr = new kscript::ExprParser(kscript::optionLanguageECMAScript);
    m_expr->setExtraLong((long)this);
    
    m_retval = new kscript::Value;


    // -- gui related --
    Application::compiletimeBind(m_expr);
    Point::compiletimeBind(m_expr);
    Size::compiletimeBind(m_expr);
    Color::compiletimeBind(m_expr);
    Console::compiletimeBind(m_expr);
    Control::compiletimeBind(m_expr);
    BannerBox::compiletimeBind(m_expr);
    Bitmap::compiletimeBind(m_expr);
    BitmapButton::compiletimeBind(m_expr);
    BorderBox::compiletimeBind(m_expr);
    BorderBoxLayout::compiletimeBind(m_expr);
    BoxLayout::compiletimeBind(m_expr);
    Brush::compiletimeBind(m_expr);
    Button::compiletimeBind(m_expr);
    zBusyCursor::compiletimeBind(m_expr);
    CheckBox::compiletimeBind(m_expr);
    ChoiceBox::compiletimeBind(m_expr);
    ColorDialog::compiletimeBind(m_expr);
    ComboBox::compiletimeBind(m_expr);
    //Cursor::compiletimeBind(m_expr);
    DialogResult::compiletimeBind(m_expr);
    Event::compiletimeBind(m_expr);
    zFont::compiletimeBind(m_expr);
    Form::compiletimeBind(m_expr);
    FormControl::compiletimeBind(m_expr);
    Graphics::compiletimeBind(m_expr);
    Label::compiletimeBind(m_expr);
    Layout::compiletimeBind(m_expr);
    Line::compiletimeBind(m_expr);
    zListBox::compiletimeBind(m_expr);
    ListViewItem::compiletimeBind(m_expr);
    ListView::compiletimeBind(m_expr);
    zMenu::compiletimeBind(m_expr);
    MenuBar::compiletimeBind(m_expr);
    OpenFileDialog::compiletimeBind(m_expr);
    PasswordEntryDialog::compiletimeBind(m_expr);
    Pen::compiletimeBind(m_expr);
    PictureBox::compiletimeBind(m_expr);
    //PrintDialog::compiletimeBind(m_expr);
    ProgressBar::compiletimeBind(m_expr);
    ProjectFileDialog::compiletimeBind(m_expr);
    RadioButton::compiletimeBind(m_expr);
    //RadioButtonGroup::compiletimeBind(m_expr);
    SaveFileDialog::compiletimeBind(m_expr);
    Slider::compiletimeBind(m_expr);
    SpinButton::compiletimeBind(m_expr);
    SpinBox::compiletimeBind(m_expr);
    zStatusBarItem::compiletimeBind(m_expr);
    zStatusBar::compiletimeBind(m_expr);
    TableBox::compiletimeBind(m_expr);
    TextBox::compiletimeBind(m_expr);
    ToolBarItem::compiletimeBind(m_expr);
    ToolBar::compiletimeBind(m_expr);
    TreeViewItem::compiletimeBind(m_expr);
    TreeView::compiletimeBind(m_expr);
    WebBrowser::compiletimeBind(m_expr);
    
    
    // -- non-gui --
    Directory::compiletimeBind(m_expr);
    DirectoryDialog::compiletimeBind(m_expr);
    DriveInfo::compiletimeBind(m_expr);
    Environment::compiletimeBind(m_expr);
    Extension::compiletimeBind(m_expr);
    File::compiletimeBind(m_expr);
    FileAccess::compiletimeBind(m_expr);
    FileDialog::compiletimeBind(m_expr);
    FileMode::compiletimeBind(m_expr);
    FileShare::compiletimeBind(m_expr);
    FileTransfer::compiletimeBind(m_expr);
    zFileInfo::compiletimeBind(m_expr);
    FileStream::compiletimeBind(m_expr);
    Base64::compiletimeBind(m_expr);
    Hash::compiletimeBind(m_expr);
    HostApp::compiletimeBind(m_expr);
    HostAutomation::compiletimeBind(m_expr);
    HostBitmap::compiletimeBind(m_expr);
    HostData::compiletimeBind(m_expr);
    HostDocument::compiletimeBind(m_expr);
    HostGlobal::compiletimeBind(m_expr);
    HostJob::compiletimeBind(m_expr);
    HostPreferences::compiletimeBind(m_expr);
    HostPrinting::compiletimeBind(m_expr);
    HostServices::compiletimeBind(m_expr);
    Log::compiletimeBind(m_expr);
    MemoryBuffer::compiletimeBind(m_expr);
    MenuItem::compiletimeBind(m_expr);
    Node::compiletimeBind(m_expr);
    NativeCall::compiletimeBind(m_expr);
    NativeType::compiletimeBind(m_expr);
    NativeModule::compiletimeBind(m_expr);
    Process::compiletimeBind(m_expr);
    ProcessOutputStream::compiletimeBind(m_expr);
    Report::compiletimeBind(m_expr);
    SeekOrigin::compiletimeBind(m_expr);
    SymmetricCrypt::compiletimeBind(m_expr);
    System::compiletimeBind(m_expr);
    SystemColors::compiletimeBind(m_expr);
    SystemMetrics::compiletimeBind(m_expr);
    SystemFonts::compiletimeBind(m_expr);
    TextReader::compiletimeBind(m_expr);
    TextWriter::compiletimeBind(m_expr);
    TextEntryDialog::compiletimeBind(m_expr);
    zTimer::compiletimeBind(m_expr);
    XmlNode::compiletimeBind(m_expr);
    HttpRequest::compiletimeBind(m_expr);

    // -- database --
    DbBulkInsert::compiletimeBind(m_expr);
    DbColumn::compiletimeBind(m_expr);
    DbDatabaseType::compiletimeBind(m_expr);
    DbEncoding::compiletimeBind(m_expr);
    DbError::compiletimeBind(m_expr);
    DbException::compiletimeBind(m_expr);
    DbObjectInfo::compiletimeBind(m_expr);
    DbType::compiletimeBind(m_expr);
    DbInsertMode::compiletimeBind(m_expr);
    DbResult::compiletimeBind(m_expr);
    DbConnection::compiletimeBind(m_expr);
    
    // -- web --
    WebDOMNode::compiletimeBind(m_expr);
    WebDOMText::compiletimeBind(m_expr);
    WebDOMAttr::compiletimeBind(m_expr);
    WebDOMElement::compiletimeBind(m_expr);
    WebDOMDocument::compiletimeBind(m_expr);
    WebDOMHTMLElement::compiletimeBind(m_expr);
    WebDOMHTMLAnchorElement::compiletimeBind(m_expr);
    WebDOMHTMLButtonElement::compiletimeBind(m_expr);
    WebDOMHTMLInputElement::compiletimeBind(m_expr);
    WebDOMHTMLLinkElement::compiletimeBind(m_expr);
    WebDOMHTMLOptionElement::compiletimeBind(m_expr);
    WebDOMHTMLParamElement::compiletimeBind(m_expr);
    WebDOMHTMLSelectElement::compiletimeBind(m_expr);
    WebDOMHTMLTextAreaElement::compiletimeBind(m_expr);
    
    
    m_script_app = Application::createObject(m_expr);
    SystemColors* system_colors = SystemColors::createObject(m_expr);
    SystemMetrics* system_metrics = SystemMetrics::createObject(m_expr);
    SystemFonts* system_fonts = SystemFonts::createObject(m_expr);

    HostServices* host_app_services = HostServices::createObject(m_expr);
    HostApp* host = HostApp::createObject(m_expr);
    HostGlobal* host_global = HostGlobal::createObject(m_expr);
    host->init(m_expr, m_script_app);

    addGuiFunction(L"print", func_print, L"*(*[*])");
    addGuiFunction(L"alert", func_alert, L"i(*[*i])");
    addGuiFunction(L"exit", func_exit, L"i(*[*])");
    addGuiFunction(L"messageBox", func_alert, L"i(*[*i])"); // deprecated
    
    m_expr->getBindVariable(L"Application")->setObject(m_script_app);
    m_expr->getBindVariable(L"SystemColors")->setObject(system_colors);
    m_expr->getBindVariable(L"SystemMetrics")->setObject(system_metrics);
    m_expr->getBindVariable(L"SystemFonts")->setObject(system_fonts);
    
    m_expr->addObjectValue(L"HostApp", host);
    m_expr->addObjectValue(L"HostGlobal", host_global);
    m_expr->addObjectValue(L"HostServices", host_app_services);
}


ScriptHost::~ScriptHost()
{
    delete m_retval;
    delete m_expr;
    delete m_pkg;
    
    std::vector<GuiMarshalSync*>::iterator it;
    for (it = m_gui_syncs.begin(); it != m_gui_syncs.end(); ++it)
    {
        delete (*it)->condition;
        delete (*it)->mutex;
        delete (*it);
    }
}

void ScriptHost::setParams(ScriptHostParams* params)
{
    if (!params)
        return;
    
    m_print_function.setValue(params->print_function);
    
    std::map<std::wstring,kscript::Value*>::iterator it;
    for (it = params->global_vars.begin(); it != params->global_vars.end(); ++it)
    {
        m_expr->addValue(it->first, *it->second);
    }
}

void ScriptHost::addValue(const std::wstring& var_name, kscript::Value& val)
{
    m_expr->addValue(var_name, val);
}


void ScriptHost::addGuiFunction(const std::wstring& func_name,
                                BindFunc func_ptr,
                                const std::wstring& formats)
{
    gui_marshal.addGuiFunction(m_expr,
                               func_name,
                               (kscript::ExprBindFunc)func_ptr,
                               formats,
                               NULL);
}

void ScriptHost::addFunction(const std::wstring& func_name,
                             BindFunc func_ptr,
                             const std::wstring& formats)
{
    m_expr->addFunction(func_name,
                        true,
                        (kscript::ExprBindFunc)func_ptr,
                        false,
                        formats,
                        NULL);
}

void ScriptHost::addIncludePath(const ScriptHostInclude& inc)
{
    m_include_paths.push_back(inc);
}

const std::vector<ScriptHostInclude>& ScriptHost::getIncludePaths()
{
    return m_include_paths;
}



GuiMarshalSync* ScriptHost::pushGuiMarshalSync()
{
    if (m_gui_sync_pos >= m_gui_syncs.size())
    {
        GuiMarshalSync* s = new GuiMarshalSync;
        s->mutex = new wxMutex;
        s->condition = new wxCondition(*(s->mutex));
        m_gui_syncs.push_back(s);
        ++m_gui_sync_pos;
        return s;
    }
    
    ++m_gui_sync_pos;
    return m_gui_syncs[m_gui_sync_pos-1];
}

void ScriptHost::popGuiMarshalSync()
{
    wxASSERT_MSG(m_gui_sync_pos > 0, wxT("Mismatched push/popGuiMarshalSync"));
    
    --m_gui_sync_pos;
}


void ScriptHost::setStartupPath(const wxString& startup_path)
{
    m_startup_path = startup_path;
}

wxString ScriptHost::getStartupPath() const
{
    return m_startup_path;
}




static bool tryIncludeFile(const wxString& filename, wxString& res_string)
{
    // -- file is not in project, try disk filesystem --

    if (!::wxFileExists(filename))
        return false;

    // -- file is not in project, try disk filesystem --
    xf_file_t f = xf_open(towstr(filename), xfOpen, xfRead, xfShareReadWrite);
    if (!f)
        return false;
        
    xf_off_t fsize = xf_get_file_size(towstr(filename));
    unsigned char* buf = new unsigned char[fsize+1];
    if (!buf)
        return false;
    xf_off_t readbytes = xf_read(f, buf, 1, fsize);
    buf[readbytes] = 0;
    
    if (readbytes >= 2 && buf[0] == 0xff && buf[1] == 0xfe)
    {
        // little endian UCS-2
        std::wstring wval;
        kl::ucsle2wstring(wval, buf+2, (readbytes-2)/2);
        res_string = wval;
    }
     else if (readbytes >= 3 && buf[0] == 0xef && buf[1] == 0xbb && buf[2] == 0xbf)
    {
        // utf-8
        wchar_t* tempbuf = new wchar_t[fsize+1];
        kl::utf8_utf8tow(tempbuf, fsize+1, (char*)buf+3, readbytes);
        res_string = tempbuf;
        delete[] tempbuf;
    }
     else
    {
        res_string = (const char*)buf;
    }
    
    delete[] buf;
    xf_close(f);
        
    return true;
}

static bool tryIncludeProjectFile(const wxString& _filename, wxString& res_string)
{
    std::wstring filename = towstr(_filename);
    
    tango::IDatabasePtr db = g_app->getDatabase();
    
    if (!db->getFileExist(filename))
        return false;
    
    

    tango::IStreamPtr stream = db->openStream(filename);
    if (stream)
    {
        std::wstring value;
           
        // -- load script data -- 
        wxMemoryBuffer buf;
        
        char* tempbuf = new char[1025];
        unsigned long read = 0;
        while (1)
        {
            if (!stream->read(tempbuf, 1024, &read))
                break;
            
            buf.AppendData(tempbuf, read);
            
            if (read != 1024)
                break;
        }
        delete[] tempbuf;
        
        
        unsigned char* ptr = (unsigned char*)buf.GetData();
        size_t buf_len = buf.GetDataLen();
        if (buf_len >= 2 && ptr[0] == 0xff && ptr[1] == 0xfe)
        {
            kl::ucsle2wstring(value, ptr+2, (buf_len-2)/2);
        }
         else if (buf_len >= 3 && ptr[0] == 0xef && ptr[1] == 0xbb && ptr[2] == 0xbf)
        {
            // utf-8
            wchar_t* tempbuf = new wchar_t[buf_len+1];
            kl::utf8_utf8tow(tempbuf, buf_len+1, (char*)ptr+3, buf_len-3);
            value = tempbuf;
            delete[] tempbuf;
        }
         else
        {
            buf.AppendByte(0);
            value = wxString::From8BitData((char*)buf.GetData());
        }
        
        res_string = value;
        return true;
    }
    
    return false;
}


static wxString chopOffPathPart(const wxString& _path)
{
    size_t len = _path.Length();
    if (len == 0)
        return _path;
    
    wxString path = _path;
    
    if (path[len-1] == wxT('\\') || path[len-1] == wxT('/'))
        path.erase(len-1, 1);
    
    size_t slash = path.find_last_of('/');
    size_t back_slash = path.find_last_of('\\');
    
    if (slash == path.npos)
        slash = back_slash;
    if (slash == path.npos)
        return wxT("");
    
    if (back_slash != path.npos && back_slash > slash)
        slash = back_slash;
        
    return path.substr(0, slash);
}

bool ScriptHost::getFullIncludePath(const wxString& include_path, ScriptHostInclude* result)
{
    wxLogNull log;
    
    if (include_path.Left(1) == wxT("/") ||
        include_path.Left(1) == wxT("\\") ||
        (include_path.Length() > 2 && include_path.GetChar(1) == wxT(':')))
    {
        // absolute path name
        
        tango::IDatabasePtr db = g_app->getDatabase();

        if (db->getFileExist(towstr(include_path)))
        {
            result->type = ScriptHostInclude::includeProjectFile;
            result->path = include_path;
            return true;
        }
         else if ((m_pkg && m_pkg->getStreamExists(include_path)) || ::wxFileExists(include_path))
        {
            result->type = ScriptHostInclude::includeFile;
            result->path = include_path;
            return true;
        }
         else
        {
            return false;
        }
    }
    
    std::vector<ScriptHostInclude> include_paths = m_include_paths;
    std::vector<ScriptHostInclude>::iterator it;
    
    if (!m_include_contexts.empty())
        include_paths.insert(include_paths.begin(), m_include_contexts.front());
        

    if (include_paths.size() == 0 && m_pkg)
    {
        ScriptHostInclude inc;
        inc.type = ScriptHostInclude::includeFile;
        inc.path = wxT("");
        include_paths.push_back(inc);
    }
    
    for (it = include_paths.begin(); it != include_paths.end(); ++it)
    {
        wxString result_path = it->path;
        wxString path = include_path;
        wxChar slash = '/';
        
        #ifdef WIN32
        if (it->type != ScriptHostInclude::includeProjectFile)
            slash = '\\';
        #endif
        
        if (m_pkg)
            slash = '/';
        
        
        while (path.length())
        {
            wxString part, part1, part2;
            part1 = path.BeforeFirst(wxT('\\'));
            part2 = path.BeforeFirst(wxT('/'));
            if (part1.Length() < part2.Length())
                part = part1;
                 else
                part = part2;
            path.erase(0, part.length());
            if (path.length() > 0)
            {
                // remove slash
                path.erase(0, 1);
            }
            
            
            if (part.Left(2) == wxT(".."))
            {
                result_path = chopOffPathPart(result_path);
            }
             else
            {
                size_t len = result_path.length();
                if (len > 0 && (result_path[len-1] == wxT('\\') || result_path[len-1] == wxT('/')))
                    result_path.erase(len-1, 1);
                
                if (m_pkg && result_path.length() == 0)
                {
                    // packages don't need a slash as the first character
                }
                 else
                {
                    result_path += slash;
                }
                
                result_path += part;
            }
        }
        
        
        if (it->type == ScriptHostInclude::includeProjectFile)
        {
            tango::IDatabasePtr db = g_app->getDatabase();
    
            if (db->getFileExist(towstr(result_path)))
            {
                result->type = ScriptHostInclude::includeProjectFile;
                result->path = result_path;
                return true;
            }
        }
         else if (it->type == ScriptHostInclude::includeFile)
        {
            if ((m_pkg && m_pkg->getStreamExists(result_path)) || ::wxFileExists(result_path))
            {
                result->type = ScriptHostInclude::includeFile;
                result->path = result_path;
                return true;
            }
        }
    }
    
    
    return false;
}


bool ScriptHost::script_host_parse_hook(kscript::ExprParseHookInfo& info)
{
    ScriptHost* host = (ScriptHost*)info.hook_param;

    if (info.element_type == kscript::ExprParseHookInfo::typeInclude)
    {
        if (host->m_include_contexts.size() > 255)
        {
            // most likely some circular include;
            return false;
        }
        
        ScriptHostInclude inc;
        if (!host->getFullIncludePath(info.expr_text, &inc))
            return false;
            
        if (inc.type == ScriptHostInclude::includeProjectFile)
        {
            wxString code;
            
            if (tryIncludeProjectFile(inc.path, code))
            {
                info.expr_text = towstr(inc.path);
                info.res_string = towstr(code);
                
                inc.path = chopOffPathPart(inc.path);
                host->m_include_contexts.push(inc);
                return true;
            }
        }
         else if (inc.type == ScriptHostInclude::includeFile)
        {
            wxString code;
            
            if (host->m_pkg)
            {
                if (!host->m_pkg->getStreamExists(inc.path))
                    return false;
                
                wxInputStream* stream = host->m_pkg->getStream(inc.path);
                if (stream)
                {
                    info.expr_text = towstr(inc.path);
                    streamToString(stream, code);
                    info.res_string = towstr(code);
                    
                    inc.path = chopOffPathPart(inc.path);
                    host->m_include_contexts.push(inc);
                    return true;
                }
            }
             else if (tryIncludeFile(inc.path, code))
            {
                info.expr_text = towstr(inc.path);
                info.res_string = towstr(code);
                
                inc.path = chopOffPathPart(inc.path);
                host->m_include_contexts.push(inc);
                return true;
            }
        }
        
        return false;
    }
     else if (info.element_type == kscript::ExprParseHookInfo::typeIncludePop)
    {
        host->m_include_contexts.pop();
        return true;
    }
    
    return false;
}

void ScriptHost::setPackage(ExtensionPkg* pkg)
{
    m_pkg = pkg;
}

ExtensionPkg* ScriptHost::getPackage() const
{
    return m_pkg;
}


bool ScriptHost::compilePackage()
{
    // a valid package file is required
    if (!m_pkg)
        return false;

    ExtensionInfo info = g_app->getExtensionMgr()->getInfo(m_pkg);
    if (!info.isOk())
        return false;
    
    // get the startup path -- if there is none specified
    // then try to load main.js by default
    wxString startup_path = info.startup_path;
    if (startup_path.IsEmpty())
        startup_path = wxT("main.js");
            
    wxInputStream* stream = m_pkg->getStream(startup_path);
    if (!stream)
        return false;
    
    wxString str;
    if (!streamToString(stream, str))
        return false;
        
    return compile(str);
}


bool ScriptHost::compile(const wchar_t* text)
{
    m_error_code = 0;
    m_error_line = 0;
    m_error_offset = 0;
    m_error_string = wxT("");
    m_error_file = wxT("");

    m_expr->setParseHook(kscript::ExprParseHookInfo::typeInclude |
                         kscript::ExprParseHookInfo::typeIncludePop,
                         script_host_parse_hook,
                         (void*)this);

    bool result = m_expr->parse(text);

    if (!result)
    {
        kscript::ExprErrorInfo error = m_expr->getErrorInfo();
        
        m_error_code = error.getCode();
        m_error_line = error.getLine();
        m_error_offset = error.getOffset();
        m_error_string = error.getText();
        m_error_file = error.getFileName();
        

        switch (m_error_code)
        {
            case kscript::errorNone:
                m_error_string = _("No error");
                break;
                
            case kscript::errorMissingOpenBrace:
                m_error_string = _("Missing open brace");
                break;

            case kscript::errorMissingCloseBrace:
                m_error_string = _("Missing close brace");
                break;
                
            case kscript::errorMissingOpenBracket:
                m_error_string = _("Missing open bracket");
                break;

            case kscript::errorMissingCloseBracket:
                m_error_string = _("Missing close bracket");
                break;
                
            case kscript::errorMissingOpenParenthesis:
                m_error_string = _("Missing open parenthesis '('");
                break;

            case kscript::errorMissingCloseParenthesis:
                m_error_string = _("Missing close parenthesis ')'");
                break;
                
            case kscript::errorMissingColon:
                m_error_string = _("Missing colon ':'");
                break;
            
            case kscript::errorMissingSemicolon:
                m_error_string = _("Missing semicolon ';'");
                break;
                
            case kscript::errorMissingQuote:
                m_error_string = _("Missing quotation mark");
                break;
                
            case kscript::errorMissingOperator:
                m_error_string = _("Missing operator");
                break;
                
            case kscript::errorMissingCatchFinally:
                m_error_string = _("Missing 'catch' or 'finally' statement");
                break;

            case kscript::errorMissingBaseClass:
                m_error_string = _("Missing base class");
                break;
                
            case kscript::errorMalformedStatement:
                m_error_string = _("Malformed statement");
                break;
                
            case kscript::errorInvalidIncludeFile:
                m_error_string = _("Missing or invalid include file");
                break;

            case kscript::errorUndeclaredIdentifier:
                m_error_string = wxString::Format(_("Undeclared identifier: '%s'"), m_error_string.c_str());
                break;
                
            case kscript::errorInvalidIdentifier:
                m_error_string = wxString::Format(_("Invalid identifier or reserved word: '%s'"), m_error_string.c_str());
                break;
            
            case kscript::errorVarRedefinition:
                m_error_string = wxString::Format(_("Variable redefinition: '%s'"), m_error_string.c_str());
                break;

            case kscript::errorInvalidParameterTypes:
                m_error_string = wxString::Format(_("Invalid parameter type(s) for function '%s'"), m_error_string.c_str());
                break;

            case kscript::errorUnterminatedString:
                m_error_string = _("Unterminated string");
                break;

            case kscript::errorMalformedNumericConstant:
                m_error_string = _("Malformed numeric constant");
                break;
            
            case kscript::errorNotInClassScope:
                m_error_string = _("This construction is only allowed in class scope");
                break;

            case kscript::errorUnknownEscapeSequence:
                m_error_string = wxString::Format(_("Unknown escape sequence '%s'"), m_error_string.c_str());
                break;
                
            case kscript::errorUnexpectedToken:
                m_error_string = _("Unexpected token");
                break;
                
            case kscript::errorSyntaxError:
                m_error_string = _("Syntax error");
                break;
            
            case kscript::errorUnknown:
            default:
                m_error_string = _("Unknown Error");
        }
    }

    return result;
}

bool ScriptHost::compile(const wxString& text)
{
#ifdef _UNICODE
    return compile((const wchar_t*)text.wc_str());
#else
    std::wstring wtext = towstr(text);
    return compile((const wchar_t*)wtext.c_str());
#endif
}

bool ScriptHost::run()
{
    m_error_code = kscript::rterrorNone;
    m_error_line = 0;
    m_error_offset = 0;
    m_error_string = wxT("");
    m_error_file = wxT("");
    
    bool res = m_expr->eval(m_retval);

    if (!res)
    {
        kscript::ExprErrorInfo error_info = m_expr->getErrorInfo();
        
        m_error_code = error_info.getCode();
        m_error_line = 0;

        switch (m_error_code)
        {
            case kscript::rterrorNone:
                m_error_string = wxT("");
                break;

            case kscript::rterrorThrownException:
                m_error_string = _("Uncaught exception.");
                break;
            
            case kscript::rterrorSyntaxError:
                m_error_string = _("Syntax error.");
                break;
            
            case kscript::rterrorTypeError:
                m_error_string = _("Type error.");
                break;
                
            case kscript::rterrorRangeError:
                m_error_string = _("Range error.");
                break;
           
            case kscript::rterrorReferenceError:
                if (error_info.getText().length() > 0)
                {
                    wxString s;
                    s.Printf(_("Reference error: while evaluating '%ls'."),
                             error_info.getText().c_str());
                    m_error_string = s;
                }
                 else
                {
                    m_error_string = _("Reference error.");
                }
                
                break;
                     
            case kscript::rterrorNoEntryPoint:
                m_error_string = _("No entry point.");
                break;
                
            case kscript::rterrorMissingObject:
                m_error_string = _("Object expected, but is missing.");
                break;
                
            case kscript::rterrorObjectTypeMismatch:
                m_error_string = _("Object type mismatch (possibly missing 'new' operator)");
                break;

            case kscript::rterrorTermNotFunction:
                m_error_string = wxString::Format(_("Term '%ls' does not evaluate to a function."),
                                                  m_expr->getErrorInfo().getText().c_str());
                break;
            
            case kscript::rterrorExit:
                m_error_code = kscript::rterrorNone;
                return true;

            case kscript::rterrorUnknown:
            default:
                m_error_string = _("Unknown Error");
        }
    }

    return res;
}

void ScriptHost::cancel()
{
    m_expr->cancel();
    m_script_app->wakeUpQueue();
}

wxString ScriptHost::getErrorString()
{
    return m_error_string;
}

wxString ScriptHost::getErrorFile()
{
    return m_error_file;
}

int ScriptHost::getErrorLine()
{
    return m_error_line;
}

size_t ScriptHost::getErrorOffset()
{
    return m_error_offset;
}

int ScriptHost::getErrorCode()
{
    return m_error_code;
}

kscript::Value* ScriptHost::getRetval()
{
    return m_retval;
}


void ScriptHost::setCurrentJob(jobs::IJobPtr job)
{
    XCM_AUTO_LOCK(m_obj_mutex);
    m_curjob = job;

    sigJobChanged();
}


jobs::IJobPtr ScriptHost::getCurrentJob()
{
    XCM_AUTO_LOCK(m_obj_mutex);
    return m_curjob;
}


