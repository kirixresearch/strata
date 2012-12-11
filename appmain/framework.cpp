/*!
 *
 * Copyright (c) 2001-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2001-11-15
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif



#include "appmain.h"
#include <kl/thread.h>
#include <queue>
#include "framework.h"
#include "framework_private.h"
#include "statusbar.h"
#include "artprovider.h"

#include <wx/aui/aui.h>
#include <wx/minifram.h>


#ifdef WIN32

    // we use a few Win32 API functions and defines directly
    // for a toolbar fix, if we are running under Win32

    #include <windows.h>
    #include <commctrl.h>

    #undef GetClassInfo
    #undef GetClassName

    #ifndef TBSTYLE_FLAT
    #define TBSTYLE_FLAT    (0x00000800L)
    #endif

    #ifndef CCS_NODIVIDER
    #define CCS_NODIVIDER   (0x00000040L)
    #endif

#endif



IFramePtr createFrameObject()
{
    return static_cast<IFrame*>(new MainFrame);
}



wxWindow* g_invisible = NULL;


// FrameCommandDispatch implementation

FrameCommandDispatch::FrameCommandDispatch()
{
    m_receiver = NULL;
    m_frame = NULL;
}

void FrameCommandDispatch::setReceiver(wxWindow* receiver)
{
    m_receiver = receiver;
}

bool FrameCommandDispatch::ProcessEvent(wxEvent& evt)
{
    bool processed = false;

    WXTYPE event_type = evt.GetEventType();

    if (event_type == wxEVT_UPDATE_UI ||
        event_type == wxEVT_COMMAND_MENU_SELECTED ||
        event_type == wxEVT_COMMAND_COMBOBOX_SELECTED ||
        event_type == wxEVT_COMMAND_TEXT_UPDATED ||
        event_type == wxEVT_COMMAND_TEXT_ENTER ||
        event_type == wxEVT_COMMAND_AUITOOLBAR_TOOL_DROPDOWN ||
        event_type == wxEVT_COMMAND_AUITOOLBAR_RIGHT_CLICK ||
        event_type == wxEVT_COMMAND_AUITOOLBAR_MIDDLE_CLICK ||
        event_type == wxEVT_COMMAND_AUITOOLBAR_OVERFLOW_CLICK ||
        event_type == wxEVT_COMMAND_AUITOOLBAR_BEGIN_DRAG)
    {
        // make sure event object is visible, if its a toolbar
        if (event_type == wxEVT_UPDATE_UI)
        {
            wxObject* evt_object = evt.GetEventObject();
            if (evt_object)
            {
                if (evt_object->IsKindOf(CLASSINFO(wxWindow)))
                {
                    wxWindow* wnd = (wxWindow*)evt_object;
                    if (!wnd->IsShown())
                    {
                        // do not process these types of commands
                        // for hidden windows
                        return true;
                    }
                }

                
                /*
                if (evt_object->IsKindOf(CLASSINFO(wxToolBar)) ||
                    evt_object->IsKindOf(CLASSINFO(wxAuiToolBar)))
                {
                    wxToolBar* tb = (wxToolBar*)evt_object;
                    if (!tb->IsShown())
                    {
                        // do not process these types of commands
                        // for hidden windows
                        return true;
                    }
                }
                */
            }
        }
         else
        {
            // non update-ui events -- (good spot for a breakpoint)
            int i = 5;
        }
        
        // let child process the command first
        if (m_receiver)
        {
            if (m_receiver->GetEventHandler()->ProcessEvent(evt))
            {
                processed = true;
            }
        }

        // give event sinks a chance, too
        if (m_frame)
        {
            wxCommandEvent* cmd = (wxCommandEvent*)&evt;
            m_frame->fire_onFrameCommand(cmd->GetId(),
                                         cmd->GetInt(),
                                         &processed);
        }
    }

    if (processed)
    {
        return true;
    }
    
    
    // Try going down the event handler chain
    if (GetNextHandler())
    {
        if (GetNextHandler()->ProcessEvent(evt))
            return true;
    }

    return false;
}





class UIContext : public IUIContext,
                  public IUIContextInternal
{
    XCM_CLASS_NAME("cfw.UIContext")
    XCM_BEGIN_INTERFACE_MAP(UIContext)
        XCM_INTERFACE_ENTRY(IUIContext)
        XCM_INTERFACE_ENTRY(IUIContextInternal)
    XCM_END_INTERFACE_MAP()

private:

    wxString m_name;
    std::vector<MenuInfo> m_menus;
    std::vector<PaneInfo> m_panes;
    wxWindow* m_pane_site_wnd;

public:

    UIContext()
    {
        m_pane_site_wnd = NULL;
    }

    virtual ~UIContext()
    {
        // delete menus before destruction
        for (std::vector<MenuInfo>::iterator it = m_menus.begin();
             it != m_menus.end(); ++it)
        {
            if (it->menu)
            {
                delete it->menu;
            }
        }
    }

    void setName(const wxString& name)
    {
        m_name = name;
    }

    wxString getName()
    {
        return m_name;
    }

    void setPaneSiteWindow(wxWindow* pane_site)
    {
        m_pane_site_wnd = pane_site;
    }

    wxWindow* getPaneSiteWindow()
    {
        return m_pane_site_wnd;
    }

    void addMenu(wxMenu* menu, const wxString& caption)
    {
        MenuInfo mi;

        mi.caption = caption;
        mi.menu = menu;

        m_menus.push_back(mi);
    }

    int getMenuCount()
    {
        return m_menus.size();
    }

    void getMenu(unsigned int idx, wxMenu*& menu, wxString& caption)
    {
        if (idx < 0 || idx >= m_menus.size())
            return;

        caption = m_menus[idx].caption.c_str();
        menu = m_menus[idx].menu;
    }

    MenuInfo& getMenuInfo(unsigned int idx)
    {
        return m_menus[idx];
    }
};





class ModelessContainer : public wxDialog,
                          public ContainerBase
{

public:

    ModelessContainer(wxWindow* parent, wxWindowID id = -1,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                unsigned flags = 0)
                    : wxDialog(parent, id, wxT(""), pos, size,
                            wxDEFAULT_DIALOG_STYLE |
                            wxRESIZE_BORDER |
                            wxSYSTEM_MENU |
                            wxNO_FULL_REPAINT_ON_RESIZE |
                            wxCLIP_CHILDREN |
                            wxWANTS_CHARS |
                            flags)
    {
        m_child = NULL;
        m_in_oncancel = false;
        m_close_called = false;
        m_min_width = -1;
        m_min_height = -1;
        m_max_width = -1;
        m_max_height = -1;

        SetWindowStyle(GetWindowStyle() & ~wxTAB_TRAVERSAL);
    }

    virtual ~ModelessContainer()
    {
        sigContainerDestructing(static_cast<wxWindow*>(this));
    }


    unsigned int getSiteType()
    {
        return sitetypeModeless | (getVisible() ? 0 : siteHidden);
    }

    void setCaption(const wxString& caption)
    {
        m_caption = caption;
        SetTitle(caption);
    }
    
    void setBitmap(const wxBitmap& bitmap)
    {
    }

    wxWindow* getWindow()
    {
        return static_cast<wxWindow*>(this);
    }

    void setVisible(bool visible)
    {
        Show(visible);
    }

    bool getVisible()
    {
        return IsShown();
    }

    void setPosition(int x, int y, int width, int height)
    {
        #ifdef WIN32
        GetParent()->ScreenToClient(&x,&y);
        #endif

        SetSize(x, y, width, height);
    }

    void getPosition(int* x, int* y, int* width, int* height)
    {
        int x1, y1, w1, h1;
        GetPosition(&x1, &y1);
        GetSize(&w1, &h1);

        #ifdef WIN32
        GetParent()->ClientToScreen(&x1, &y1);
        #endif

        if (x) *x = x1;
        if (y) *y = y1;
        if (width) *width = w1;
        if (height) *height = h1;
    }

    void setMinSize(int width, int height)
    {
        m_min_width = width;
        m_min_height = height;

        SetSizeHints(m_min_width,
                     m_min_height,
                     m_max_width,
                     m_max_height);
    }

    void setMaxSize(int width, int height)
    {
        m_max_width = width;
        m_max_height = height;

        SetSizeHints(m_min_width,
                     m_min_height,
                     m_max_width,
                     m_max_height);
    }

    void setDocument(IDocumentPtr document)
    {  
        m_document = document;

        m_child = NULL;
        if (m_document.isOk())
        {
            m_child = m_document->getDocumentWindow();
            wxSize size = GetClientSize();
            m_child->SetSize(0, 0, size.GetWidth(), size.GetHeight());
        }
    }


    void onSize(wxSizeEvent& evt)
    {
        if (m_child)
        {
            wxSize size = GetClientSize();
            m_child->SetSize(0, 0, size.GetWidth(), size.GetHeight());
        }
    }

    void onClose(wxCloseEvent& evt)
    {
        if (m_document.isNull())
            return;

        bool res = m_document->onSiteClosing(evt.CanVeto() ? false : true);
        if (evt.CanVeto() && !res)
        {
            evt.Veto();
            return;
        }

        Destroy();
    }


    bool closeContainer(bool force = false)
    {
        // a programmatic call to Close() is fine, but
        // it means we shouldn't send an on cancel to the
        // 'dialog' child
        
        m_close_called = true;
        
        SetEscapeId(wxID_NONE);
        
        return Close(force);
    }


    void onCancel(wxCommandEvent& event)
    {
        if (m_in_oncancel)
            return;

        m_in_oncancel = true;

        if (m_child && !m_close_called)
        {        
            wxCommandEvent event(wxEVT_COMMAND_BUTTON_CLICKED, wxID_CANCEL);
            event.SetEventObject(m_child);
            m_child->GetEventHandler()->ProcessEvent(event);
        }

        Destroy();

        m_in_oncancel = false;
    }

private:

    wxString m_caption;
    int m_id;
    IDocumentPtr m_document;
    wxWindow* m_child;
    bool m_in_oncancel;
    bool m_close_called;    // window::Close() was called explicitly

    int m_min_width;
    int m_min_height;
    int m_max_width;
    int m_max_height;

    DECLARE_EVENT_TABLE()
    DECLARE_CLASS(ModelessContainer)
};

IMPLEMENT_CLASS(ModelessContainer, wxDialog)

BEGIN_EVENT_TABLE(ModelessContainer, wxDialog)
    EVT_SIZE(ModelessContainer::onSize)
    EVT_CLOSE(ModelessContainer::onClose)
    EVT_BUTTON(wxID_CANCEL, ModelessContainer::onCancel)
END_EVENT_TABLE()




class MiniModelessContainer : public wxFrame,
                              public ContainerBase
{

public:

    MiniModelessContainer(wxWindow* parent, wxWindowID id = -1,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize)
                    : wxFrame(parent, id, wxT(""), pos, size,
                            wxDEFAULT_FRAME_STYLE |
                            wxRESIZE_BORDER |
                            wxSYSTEM_MENU |
                            wxFRAME_NO_TASKBAR |
                            wxNO_FULL_REPAINT_ON_RESIZE |
                            wxFRAME_FLOAT_ON_PARENT |
                            wxFRAME_TOOL_WINDOW |
                            wxCLIP_CHILDREN)
    {
        m_child = NULL;
        m_id = 0;
        SetWindowStyle(GetWindowStyle() & ~wxTAB_TRAVERSAL);
        
        m_min_width = -1;
        m_min_height = -1;
        m_max_width = -1;
        m_max_height = -1;
    }

    ~MiniModelessContainer()
    {
        sigContainerDestructing(static_cast<wxWindow*>(this));
    }



    unsigned int getSiteType()
    {
        return sitetypeMiniModeless | (getVisible() ? 0 : siteHidden);
    }

    void setCaption(const wxString& caption)
    {
        m_caption = caption;
        SetTitle(caption);
    }

    void setBitmap(const wxBitmap& bitmap)
    {
    }
    
    wxWindow* getWindow()
    {
        return static_cast<wxWindow*>(this);
    }

    void setVisible(bool visible)
    {
        Show(visible);
    }

    bool getVisible()
    {
        return IsShown();
    }

    void setPosition(int x, int y, int width, int height)
    {
        SetSize(x, y, width, height);
    }

    void getPosition(int* x, int* y, int* width, int* height)
    {
        int x1, y1, w1, h1;
        GetPosition(&x1, &y1);
        GetSize(&w1, &h1);
        if (x) *x = x1;
        if (y) *y = y1;
        if (width) *width = w1;
        if (height) *height = h1;
    }

    void setMinSize(int width, int height)
    {
        m_min_width = width;
        m_min_height = height;

        SetSizeHints(m_min_width,
                     m_min_height,
                     m_max_width,
                     m_max_height);
    }

    void setMaxSize(int width, int height)
    {
        m_max_width = width;
        m_max_height = height;

        SetSizeHints(m_min_width,
                     m_min_height,
                     m_max_width,
                     m_max_height);
    }

    void setDocument(IDocumentPtr document)
    {
        m_document = document;

        m_child = NULL;
        if (m_document.isOk())
        {
            m_child = m_document->getDocumentWindow();
            wxSize size = GetClientSize();
            m_child->SetSize(0, 0, size.GetWidth(), size.GetHeight());
        }
    }

    bool closeContainer(bool force = false)
    {
        return Close(force);
    }


    void onSize(wxSizeEvent& evt)
    {
        if (m_child)
        {
            wxSize size = GetClientSize();
            m_child->SetSize(0, 0, size.GetWidth(), size.GetHeight());
        }
    }

    void onClose(wxCloseEvent& evt)
    {
        if (m_document.isNull())
            return;

        bool res = m_document->onSiteClosing(evt.CanVeto() ? false : true);
        if (evt.CanVeto() && !res)
        {
            evt.Veto();
            return;
        }

        evt.Skip();
    }


private:

    wxString m_caption;
    int m_id;
    IDocumentPtr m_document;
    wxWindow* m_child;

    int m_min_width;
    int m_min_height;
    int m_max_width;
    int m_max_height;

    DECLARE_EVENT_TABLE()
    DECLARE_CLASS(MiniModelessContainer)
};

IMPLEMENT_CLASS(MiniModelessContainer, wxFrame)

BEGIN_EVENT_TABLE(MiniModelessContainer, wxFrame)
    EVT_SIZE(MiniModelessContainer::onSize)
    EVT_CLOSE(MiniModelessContainer::onClose)
END_EVENT_TABLE()





class PanelContainer : public wxPanel,
                       public ContainerBase
{
public:

    PanelContainer(wxWindow* parent) : wxPanel(parent,
                                               -1,
                                               wxPoint(0,0),
                                               wxSize(1,1),
                                               wxCLIP_CHILDREN)
    {
        SetWindowStyle(GetWindowStyle() & ~wxTAB_TRAVERSAL);
        m_child = NULL;
        m_min_width = -1;
        m_min_height = -1;
        m_max_width = -1;
        m_max_height = -1;
    }

    ~PanelContainer()
    {
        sigContainerDestructing(static_cast<wxWindow*>(this));
    }


    unsigned int getSiteType()
    {
        wxAuiPaneInfo& pane = m_mgr->GetPane(this);
        if (!pane.IsOk())
            return sitetypeDockable;
            
        unsigned int site_type = 0;
        switch (pane.dock_direction)
        {
            case wxAUI_DOCK_TOP:    site_type = dockTop; break;
            case wxAUI_DOCK_LEFT:   site_type = dockLeft; break;
            case wxAUI_DOCK_BOTTOM: site_type = dockBottom; break;
            case wxAUI_DOCK_RIGHT:  site_type = dockRight; break;
        }

        return sitetypeDockable |
               (pane.IsFloating() ? dockFloating : site_type) |
               (pane.IsShown() ? 0 : siteHidden) |
               (pane.IsFixed() ? siteNoResize : 0);
    }


    void setCaption(const wxString& caption)
    {
        wxAuiPaneInfo& pane = m_mgr->GetPane(this);
        if (pane.IsOk())
        {
            pane.Caption(caption);
            m_mgr->Update();
        }
    }

    void setBitmap(const wxBitmap& bitmap)
    {
    }

    wxWindow* getWindow()
    {
        return static_cast<wxWindow*>(this);
    }

    void setVisible(bool visible)
    {
        wxAuiPaneInfo& pane = m_mgr->GetPane(this);
        if (pane.IsOk())
        {
            pane.Show(visible);
            m_mgr->Update();
        }
    }

    bool getVisible()
    {
        wxAuiPaneInfo& pane = m_mgr->GetPane(this);
        if (pane.IsOk())
            return pane.IsShown();
        return false;
    }

    void setPosition(int x, int y, int width, int height)
    {
        wxAuiPaneInfo& pane = m_mgr->GetPane(this);
        if (pane.IsOk())
        {
            pane.FloatingPosition(x,y);
            pane.FloatingSize(width, height);
            m_mgr->Update();
        }
    }

    void getPosition(int* x, int* y, int* width, int* height)
    {
        int x1 = 0;
        int y1 = 0;
        int w1 = 0;
        int h1 = 0;

        wxAuiPaneInfo& pane = m_mgr->GetPane(this);
        if (pane.IsOk())
        {
            if (pane.IsFloating())
            {
                x1 = pane.floating_pos.x;
                y1 = pane.floating_pos.y;
                w1 = pane.floating_size.x;
                h1 = pane.floating_size.y;
            }
             else
            {
                y1 = pane.dock_pos;
                GetClientSize(&w1, &h1);
            }
        }
         else
        {
            GetClientSize(&w1, &h1);
        }

        if (x) *x = x1;
        if (y) *y = y1;
        if (width) *width = w1;
        if (height) *height = h1;
    }
    
    void setDocument(IDocumentPtr document)
    {
        m_document = document;
        m_child = NULL;
        
        if (m_document.isOk())
        {
            m_child = m_document->getDocumentWindow();
            wxSize size = GetClientSize();
            m_child->SetSize(0, 0, size.GetWidth(), size.GetHeight());
        }
    }


    void setMinSize(int width, int height)
    {
        m_min_width = width;
        m_min_height = height;

        SetSizeHints(m_min_width,
                     m_min_height,
                     m_max_width,
                     m_max_height);
    }

    void setMaxSize(int width, int height)
    {
        m_max_width = width;
        m_max_height = height;

        SetSizeHints(m_min_width,
                     m_min_height,
                     m_max_width,
                     m_max_height);
    }

    bool closeContainer(bool force = false)
    {
        return false;
    }

    void onClose(wxCloseEvent& evt)
    {
        if (m_document.isNull())
            return;
            
        bool res = m_document->onSiteClosing(evt.CanVeto() ? false : true);
        if (evt.CanVeto() && !res)
        {
            evt.Veto();
            return;
        }

        evt.Skip();
    }


    void onSize(wxSizeEvent& evt)
    {
        wxWindow* wnd = getDocumentWindow();
        
        if (wnd)
        {
            int w, h;
            GetClientSize(&w, &h);
            wnd->SetSize(0, 0, w, h);
        }
    }

public:

    wxAuiManager* m_mgr;
    IDocumentPtr m_document;
    wxWindow* m_child;

    int m_min_width;
    int m_min_height;
    int m_max_width;
    int m_max_height;

    DECLARE_EVENT_TABLE()
    DECLARE_CLASS(PanelContainer)
};

IMPLEMENT_CLASS(PanelContainer, wxPanel)

BEGIN_EVENT_TABLE(PanelContainer, wxPanel)
    EVT_SIZE(PanelContainer::onSize)
END_EVENT_TABLE()




ContainerBase* getContainerFromWnd(wxWindow* wnd)
{
    if (!wnd)
        return NULL;
        
    if (wnd->IsKindOf(CLASSINFO(ChildFrame)))
    {
        return (ContainerBase*)(ChildFrame*)wnd;
    }
     else if (wnd->IsKindOf(CLASSINFO(PanelContainer)))
    {
        return (ContainerBase*)(PanelContainer*)wnd;
    }
     else if (wnd->IsKindOf(CLASSINFO(ModelessContainer)))
    {
        return (ContainerBase*)(ModelessContainer*)wnd;
    }
     else if (wnd->IsKindOf(CLASSINFO(MiniModelessContainer)))
    {
        return (ContainerBase*)(MiniModelessContainer*)wnd;
    }
    
    return NULL;
}

ContainerBase* getContainerFromSite(IDocumentSitePtr site)
{
    wxWindow* wnd = NULL;
    
    if (site.isOk())
        wnd = site->getContainerWindow();
    
    return getContainerFromWnd(wnd);
}




// DocumentDeleteWatcher

// this class watches for the document window destruction.  This is facilitated
// by wxWidgets wxClientData class and Set/GetClientObject().  The document's
// SetClientObject() will be called with this object, and as soon as the document
// is destroyed, the object will too

class DocumentDeleteWatcher : public wxClientData
{
public:

    ~DocumentDeleteWatcher()
    {
        sigDocumentDeleting();
    }
    
    xcm::signal0 sigDocumentDeleting;
};








// DocumentSite implementation

class DocumentSite : public IDocumentSite,
                     public xcm::signal_sink
{
    XCM_CLASS_NAME("cfw.DocumentSite")
    XCM_BEGIN_INTERFACE_MAP(DocumentSite)
        XCM_INTERFACE_ENTRY(IDocumentSite)
    XCM_END_INTERFACE_MAP()

public:

    DocumentSite()
    {
        m_id = 0;
        m_wnd = NULL;
        m_cont = NULL;
        m_min_width = -1;
        m_min_height = -1;
        m_max_width = -1;
        m_max_height = -1;
        m_type = sitetypeNormal;
        m_doc_delete_watcher = NULL;
    }
    
    virtual ~DocumentSite()
    {
        m_doc.p = NULL;
    }
    
    void setDocument(IDocumentPtr doc)
    {
        // check for same document
        if (m_doc.p == doc.p)
            return;
            
        // remove any delete watcher for the old document
        // the delete watcher object itself will be deleted whenever
        // the old document's window object is destroyed, so we
        // don't need to worry about that
        if (m_doc_delete_watcher)
        {
            m_doc_delete_watcher->sigDocumentDeleting.disconnect();
        }
            
        // assign the new document pointer
        m_doc.p = NULL;
        m_doc = doc;
        
        // if we are storing a document pointer, we need to initialize
        // a new document delete watcher
        if (doc)
        {
            wxWindow* doc_wnd = doc->getDocumentWindow();
            
            if (doc_wnd)
            {
                m_doc_delete_watcher = new DocumentDeleteWatcher;
                
                m_doc_delete_watcher->sigDocumentDeleting.connect(
                                           this,
                                           &DocumentSite::onDocumentDeleting);
                                           
                doc_wnd->SetClientObject(m_doc_delete_watcher);
            }
        }
    }
    
    IDocumentPtr getDocument() 
    {
        return m_doc;
    }
    
    void setContainer(ContainerBase* container)
    {
        m_cont = container;
        m_wnd = container->getWindow();
    }
    
    wxWindow* getContainerWindow()
    {
        return m_wnd;
    }

    IUIContextPtr getUIContext()
    {
        return m_ui_context;
    }

    void setUIContext(IUIContextPtr ui_context)
    {
        m_ui_context = ui_context;
    }

    void setSiteType(int type)
    {
        m_type = (type & sitetypeAll);
    }
    
    unsigned int getSiteType()
    {
        return m_type;
    }

    void setId(int id)
    {
        m_id = id;
    }
    
    int getId()
    {
        return m_id;
    }

    wxString getName()
    {
        return m_site_name;
    }
    
    void setName(const wxString& name)
    {
        m_site_name = name;
        
        if (m_cont)
        {
            m_cont->setName(name);
        }
    }

    void setCaption(const wxString& caption)
    {
        m_caption = caption;
        
        if (m_cont && isInPlaceActive())
        {
            m_cont->setCaption(caption);
        }
    }
    
    wxString getCaption()
    {
        return m_caption;
    }
    
    wxBitmap getBitmap()
    {
        return m_bitmap;
    }
    
    void setBitmap(const wxBitmap& bitmap)
    {
        m_bitmap = bitmap;
        
        if (m_cont && isInPlaceActive())
        {
            m_cont->setBitmap(bitmap);
        }
    }
    
    bool getVisible()
    {
        if (!m_wnd)
            return false;
            
        if (m_cont && !m_cont->getVisible())
            return false;
        
        return m_wnd->IsShown();
    }
    
    void setVisible(bool visible)
    {
        if (m_cont && isInPlaceActive())
        {
            m_cont->setVisible(visible);
        }
    }

    void setPosition(int x, int y, int width, int height)
    {
        if (m_cont && isInPlaceActive())
        {
            m_cont->setPosition(x, y, width, height);
        }
    }
    
    void getPosition(int* x, int* y, int* width, int* height)
    {
        if (m_cont)
        {
            m_cont->getPosition(x, y, width, height);
        }
         else
        {
            if (x) *x = 0;
            if (y) *y = 0;
            if (width) *width = 0;
            if (height) *height = 0;
        }
    }

    void setMinSize(int width, int height)
    {
        m_min_width = width;
        m_min_height = height;

        m_wnd->SetSizeHints(m_min_width,
                            m_min_height,
                            m_max_width,
                            m_max_height);
    }
    
    void setMaxSize(int width, int height)
    {
        m_max_width = width;
        m_max_height = height;

        m_wnd->SetSizeHints(m_min_width,
                            m_min_height,
                            m_max_width,
                            m_max_height);
    }

private:

    bool isInPlaceActive()
    {
        if (!m_cont)
            return false;
        
        wxWindow* doc_wnd = getDocumentWindow();
        wxWindow* active_doc_wnd = m_cont->getDocumentWindow();
        
        return (m_doc.isOk() && (active_doc_wnd == doc_wnd)) ? true : false;
    }
    
    wxWindow* getDocumentWindow()
    {
        if (!m_doc)
            return NULL;
            
        return m_doc->getDocumentWindow();
    }
    
    void onDocumentDeleting()
    {
        m_doc.p = NULL;
        m_doc_delete_watcher = NULL;
    }
    
private:


    IDocumentPtr m_doc;
    IUIContextPtr m_ui_context;
    wxString m_caption;
    wxBitmap m_bitmap;
    wxString m_site_name;
    int m_id;
    wxWindow* m_wnd;
    ContainerBase* m_cont;

    int m_min_width;
    int m_min_height;
    int m_max_width;
    int m_max_height;
    
    int m_type;
    
    DocumentDeleteWatcher* m_doc_delete_watcher;
};





// ChildFrame implementation

IMPLEMENT_CLASS(ChildFrame, wxMDIChildFrame)

BEGIN_EVENT_TABLE(ChildFrame, wxMDIChildFrame)
    EVT_SIZE(ChildFrame::onSize)
    EVT_ACTIVATE(ChildFrame::onActivate)
    EVT_CLOSE(ChildFrame::onClose)
    EVT_MOVE(ChildFrame::onMove)
END_EVENT_TABLE()


ChildFrame::ChildFrame(wxMDIParentFrame* parent,
                       wxWindowID id,
                       const wxString& title,
                       const wxPoint& pos,
                       const wxSize& size,
                       long style,
                       const wxString& name) :
                            wxMDIChildFrame(parent,
                                            id,
                                            title,
                                            pos,
                                            size,
                                            style,
                                            name)
{
    SetExtraStyle(GetExtraStyle() | wxWS_EX_BLOCK_EVENTS);
    m_parent = parent;
    m_closing = false;
    m_id = 0;
}


ChildFrame::~ChildFrame()
{
    sigContainerDestructing(static_cast<wxWindow*>(this));
}


bool ChildFrame::closeContainer(bool force)
{
    return Close(force);
}


wxWindow* ChildFrame::getWindow()
{
    return static_cast<wxWindow*>(this);
}
    
unsigned int ChildFrame::getSiteType()
{
    return sitetypeNormal |
           (getVisible() ? 0 : siteHidden) |
           (IsMaximized() ? 0 : siteMaximized);
}

void ChildFrame::setCaption(const wxString& caption)
{
    SetTitle(caption);
}

void ChildFrame::setBitmap(const wxBitmap& bitmap)
{
#ifdef CFW_USE_TABMDI
    wxAuiMDIParentFrame* frame = this->GetMDIParentFrame();
    wxAuiMDIClientWindow* cli_window = frame->GetClientWindow();
    if (cli_window != NULL)
    {
        int idx = cli_window->GetPageIndex(this);
        if (idx != -1)
            cli_window->SetPageBitmap((size_t)idx, bitmap);
    }
#else
    wxIcon icon;
    icon.CopyFromBitmap(bitmap);
    SetIcon(icon);
#endif
}

wxString ChildFrame::getCaption()
{
    return GetTitle();
}

bool ChildFrame::closeSite(bool force)
{
    if (!Close(force))
    {
        return false;
    }

    Show(false);    // hide the site

    ::wxYield();

    return true;
}




IUIContextPtr ChildFrame::getUIContext()
{
    IDocumentSitePtr site = getActiveSite();
    if (site.isNull())
        return xcm::null;
        
    return site->getUIContext();
}




void ChildFrame::onSize(wxSizeEvent& evt)
{
    wxWindow* wnd = getDocumentWindow();
    if (wnd)
    {
        wnd->SetSize(GetClientSize());
    }

    evt.Skip();
}


void ChildFrame::onActivate(wxActivateEvent& evt)
{
    if (evt.GetActive())
    {
        //wxString s = wxT("Activated - ");
        //s += GetTitle();
        //s += wxT("\n");
        //OutputDebugString(s.c_str());

        sigFrameActivated(this);
    }
     else
    {
        if (!m_closing)
        {
            //wxString s = wxT("DeActivated - ");
            //s += GetTitle();
            //s += wxT("\n");
            //OutputDebugString(s.c_str());

            sigFrameDeactivated(this);
        }
    }
}

void ChildFrame::onClose(wxCloseEvent& evt)
{
    IDocumentPtr doc = getDocument();
    
    if (doc.isOk())
    {
        bool result = doc->onSiteClosing(evt.CanVeto() ? false : true);
        doc.clear();
        
        if (!result)
        {
            if (evt.CanVeto())
            {
                evt.Veto();
                return;
            }
        }
    }

    Show(false);

    // At least under win32, the child window must be closed before
    // deactivation.  Otherwise, menu un-merging happens incorrectly
    // because all menubar items are offset by 1 (due to the document
    // icon being menubar item zero)

    if (IsMaximized())
    {
        Maximize(false);
    }

    sigFrameDestructing(this);
    m_closing = true;

#ifndef CFW_USE_TABMDI
    m_parent->ActivateNext();
#endif
    Destroy();
}

void ChildFrame::onMove(wxMoveEvent& evt)
{
    sigFrameMoved(this);
}

void ChildFrame::setVisible(bool visible)
{
    Show(visible);
}

bool ChildFrame::getVisible()
{
    return IsShown() ? true : false;
}

void ChildFrame::setPosition(int x, int y, int width, int height)
{
    SetSize(x, y, width, height);
}

void ChildFrame::getPosition(int* x, int* y, int* width, int* height)
{
    int x1, y1, w1, h1;
    GetPosition(&x1, &y1);
    GetSize(&w1, &h1);
    if (x) *x = x1;
    if (y) *y = y1;
    if (width) *width = w1;
    if (height) *height = h1;
}

void ChildFrame::setMinSize(int width, int height)
{
}

void ChildFrame::setMaxSize(int width, int height)
{
}



// wxCfwToolBar is a fake derived class to allow access to m_tools

class wxCfwToolBar : public wxToolBar
{
public:
    wxToolBarToolsList& getToolsList() { return m_tools; }
};





class SizingHook : public wxEvtHandler
{
public:

    xcm::signal0 sigOnSizeEvent;

public:
    
    bool ProcessEvent(wxEvent& evt)
    {
        bool result = GetNextHandler()->ProcessEvent(evt);

        if (evt.GetEventType() == wxEVT_SIZE)
        {
            sigOnSizeEvent();
        }

        return result;
    }
};


// MainFrame implementation


BEGIN_EVENT_TABLE(MainFrame, wxMDIParentFrame)
    EVT_IDLE(MainFrame::onIdle)
    EVT_CLOSE(MainFrame::onCloseEvent)
    //EVT_MENU_OPEN(MainFrame::onMenuOpen)  // causing problems with UpdateUI handlers not being called in appcontroller (with wx 2.9)
    EVT_SIZE(MainFrame::onSize)
    EVT_AUI_PANE_CLOSE(MainFrame::onAuiPaneClose)
    EVT_AUINOTEBOOK_TAB_RIGHT_UP(-1, MainFrame::onChildRightClick)
END_EVENT_TABLE()


MainFrame::MainFrame()
{
    m_tile_lock = TileLock_None;
    m_menu_merge_count = 0;
    m_menu_merge_position = 0;
    m_statusbar = xcm::null;
    m_active_ui = xcm::null;
    m_active_child = NULL;
    m_last_child = NULL;
    m_invisible = NULL;
    m_unique_id = 0;
}

MainFrame::~MainFrame()
{
    m_isBeingDeleted = true;

    GetClientWindow()->PopEventHandler(true);


#if wxCHECK_VERSION(2,9,0)
    if (m_dispatcher_child && !m_dispatcher_child->IsUnlinked())
        this->RemoveEventHandler(m_dispatcher_child);
    
    if (m_dispatcher_frameevt && !m_dispatcher_frameevt->IsUnlinked())
        this->RemoveEventHandler(m_dispatcher_frameevt);

    std::vector<wxEvtHandler*>::reverse_iterator rit;
    for (rit = m_winevent_handlers.rbegin();
         rit != m_winevent_handlers.rend(); ++rit)
    {
        if (!(*rit)->IsUnlinked())
            this->RemoveEventHandler(*rit);
    }

#endif
    
    delete m_dispatcher_child;
    delete m_dispatcher_frameevt;

}


bool MainFrame::create(wxWindow* parent,
                       const wxString& caption,
                       int x,
                       int y,
                       int width,
                       int height)
{
    // first, create our window
    if (!Create(parent, -1, caption, wxPoint(x,y), wxSize(width, height),
                   wxDEFAULT_FRAME_STYLE |
                   wxFRAME_NO_WINDOW_MENU |
                   wxCLIP_CHILDREN |
                   wxNO_FULL_REPAINT_ON_RESIZE))
    {
        return false;
    }

    m_invisible = new wxWindow(this, -1);
    m_invisible->Show(false);
    g_invisible = m_invisible;

    wxMDIClientWindow* mdi_client = GetClientWindow();

/*
#ifdef __WXMSW__
    LONG l = GetWindowLongA((HWND)mdi_client->GetHWND(), GWL_EXSTYLE);
    l &= ~WS_EX_CLIENTEDGE;
    SetWindowLongA((HWND)mdi_client->GetHWND(), GWL_EXSTYLE, l);
#endif
*/

    SizingHook* size_hook = new SizingHook;
    size_hook->sigOnSizeEvent.connect(this, &MainFrame::onMdiClientSize);
    mdi_client->PushEventHandler(size_hook);

    m_mgr.SetManagedWindow(this);
    m_mgr.SetArtProvider(new CfwDockArt);
    
#ifdef CFW_USE_XMDI
    if (!m_mgr.GetPane(GetClientWindow()).IsOk())
    {
        m_mgr.AddPane(GetClientWindow(),
                      wxAuiPaneInfo().Name(wxT("mdiclient")).
                      CenterPane().PaneBorder(false));
    }
#endif


    // set manager flags
    int flags;
    flags = 0;
    flags |= wxAUI_MGR_TRANSPARENT_DRAG;
    flags |= wxAUI_MGR_ALLOW_ACTIVE_PANE;

    // NOTE: GetMetric() takes a pointer to a window as a second optional parameter;
    // to get the caption size of a floating window, we should pass a pointer to a 
    // floating window, otherwise the caption size returned will be that of the top 
    // level window; may want to put this in the art provider rather than here
    
    //int caption_size = wxSystemSettings::GetMetric(wxSYS_CAPTION_Y);
    m_mgr.GetArtProvider()->SetMetric(wxAUI_DOCKART_CAPTION_SIZE, 21);
    m_mgr.SetFlags(m_mgr.GetFlags() ^ flags);
    
    m_mgr.Update();

    // set notebook flags
    flags = 0;
    flags |= wxAUI_NB_WINDOWLIST_BUTTON;
    flags |= wxAUI_NB_TAB_FIXED_WIDTH;
    //flags |= wxAUI_NB_CLOSE_ON_ALL_TABS;
    
    size_t i, count;
    wxAuiPaneInfoArray& all_panes = m_mgr.GetAllPanes();
    for (i = 0, count = all_panes.GetCount(); i < count; ++i)
    {
        wxAuiPaneInfo& pane = all_panes.Item(i);

        if (pane.window->IsKindOf(CLASSINFO(wxAuiNotebook)))
        {
            wxAuiNotebook* nb = (wxAuiNotebook*)pane.window;
            nb->SetArtProvider(new CfwTabArt);
            nb->SetWindowStyleFlag(nb->GetWindowStyleFlag() ^ flags);
            nb->Refresh();
        }
    }
    
    // add event handler

    m_dispatcher_child = new FrameCommandDispatch;

    m_dispatcher_frameevt = new FrameCommandDispatch;
    m_dispatcher_frameevt->m_frame = this;

    updateWindowEventHandlers();

    return true;
}




class wxAppHack : public wxApp
{
public:
    // delete all objects in wxPendingDelete list
    void deletePendingObjects()
    {
        DeletePendingObjects();
    }
};

void MainFrame::onCloseEvent(wxCloseEvent& evt)
{
    fire_onFrameClose(evt);
    if (evt.GetVeto())
        return;

    //  close out all child windows
    closeAll(true);
    ::wxYield();
    wxAppHack* app = (wxAppHack*)wxTheApp;
    app->deletePendingObjects();

    // if a child refused to close, don't close frame
    if (GetActiveChild())
        return;


    // make sure we don't get any more signals
    disconnectAllSignals();

    // notify everybody that there is no more active container
    m_active_ui = xcm::null;
    fire_onActiveChildChanged(xcm::null);
    dispatchAllEvents();


    // deinitialize wxAUI
    m_mgr.UnInit();
    
    // destroy the main frame
    Destroy();
}


bool MainFrame::Destroy()
{
    fire_onFrameDestroy();

    return wxFrame::Destroy();
}

void MainFrame::onSize(wxSizeEvent& evt)
{
    fire_onFrameSize(evt);
}

void MainFrame::onChildRightClick(wxAuiNotebookEvent& evt)
{
#ifdef CFW_USE_TABMDI
    int selection = evt.GetSelection();
    if (selection < 0)
        return;
    ChildFrame* child = (ChildFrame*)GetClientWindow()->GetPage((size_t)selection);
    if (child)
    {
        IDocumentSitePtr site = lookupSiteByContainer(child);
        if (site.isOk())
        {
            sigSiteContextMenu().fire(site);
        }
    }
#endif
}

void MainFrame::onAuiPaneClose(wxAuiManagerEvent& evt)
{
    wxWindow* pane = evt.GetPane()->window;
    if (pane->IsKindOf(CLASSINFO(PanelContainer)))
    {
        PanelContainer* panel = (PanelContainer*)pane;
        
        wxCloseEvent close_evt;
        close_evt.SetCanVeto(evt.CanVeto());
        panel->onClose(close_evt);
        if (evt.GetVeto())
            evt.Veto();
    }
}

void MainFrame::onMenuOpen(wxMenuEvent& evt)
{
    wxMenuBar* frame_menubar = GetMenuBar();
    if (!frame_menubar)
        return;
        
    int menu_count = frame_menubar->GetMenuCount();
    for (int i = 0; i < menu_count; ++i)
    {
        wxMenu* menu = frame_menubar->GetMenu(i);
        if (menu)
        {
            menu->UpdateUI(this);
        }
    }
}


wxFrame* MainFrame::getFrameWindow()
{
    return static_cast<wxFrame*>(this);
}

wxAuiManager& MainFrame::getAuiManager()
{
    return m_mgr;
}

void MainFrame::setStatusBar(IStatusBarPtr statusbar)
{
    // remove any previous statusbar that may have existed
    if (m_statusbar.isOk())
        m_statusbar.clear();
    
    m_statusbar = statusbar;
}

IStatusBarPtr MainFrame::getStatusBar()
{
    return m_statusbar;
}

wxWindow* MainFrame::getInvisibleWindow()
{
    return m_invisible;
}

void MainFrame::moveWindow(int x, int y, int width, int height)
{
    SetSize(x, y, width, height);
}

void MainFrame::setCaption(const wxString& caption)
{
    SetTitle(caption);
}

void MainFrame::setIcon(const wxIcon& icon)
{
    SetIcon(icon);
}

void MainFrame::setIcons(const wxIconBundle& icons)
{
    SetIcons(icons);
}


void MainFrame::show(bool visible)
{
    Show(visible);
    
#ifdef __WXGTK__
#if !wxCHECK_VERSION(2,9,0)
    UpdateMenuBarSize();
#endif
#endif
}




IUIContextPtr MainFrame::createUIContext(const wxString& name)
{
    UIContext* ui_context = new UIContext;
    ui_context->setName(name);
    ui_context->setPaneSiteWindow(m_invisible);

    IUIContextPtr ui = static_cast<IUIContext*>(ui_context);
    m_ui_contexts.push_back(ui);

    return ui;
}

IUIContextPtr MainFrame::lookupUIContext(const wxString& name)
{
    std::vector<IUIContextPtr>::iterator it;
    for (it = m_ui_contexts.begin(); it != m_ui_contexts.end(); ++it)
    {
        if (!name.CmpNoCase((*it)->getName()))
            return *it;
    }

    return xcm::null;
}

void MainFrame::setMenuBar(wxMenuBar* menubar, int menu_merge_position)
{
    SetMenuBar(menubar);
    m_menu_merge_position = menu_merge_position;
}

void MainFrame::prepareToolBar(wxToolBar* toolbar)
{
    #ifdef WIN32
    HWND tbwnd = (HWND)toolbar->GetHandle();
    LONG newstyle = GetWindowLongA(tbwnd, GWL_STYLE);
    newstyle &= ~WS_BORDER;
    newstyle |= (TBSTYLE_FLAT | CCS_NODIVIDER);
    SetWindowLongA(tbwnd, GWL_STYLE, newstyle);
    #endif
}






void MainFrame::dockWindow(wxWindow* wnd,
                           const wxString& caption,
                           int site_type,
                           int row,
                           int offset,
                           int width,
                           int height)
{
    int floating_x, floating_y;

    if (row == -1)
        row = 0;

    if (site_type & dockFloating)
    {
        floating_x = row;
        floating_y = offset;
        row = 0;
        offset = 0;
    }

    if (width == 0 && height == 0)
    {
        wnd->GetClientSize(&width, &height);
        if (wnd->IsKindOf(CLASSINFO(wxToolBar)))
        {
            wxCfwToolBar* toolbar = (wxCfwToolBar*)wnd;
            wxToolBarToolsList& tools = toolbar->getToolsList();

            wxSize tool_bitmap_size = toolbar->GetToolBitmapSize();
            if (tool_bitmap_size.x == 0)
            {
                tool_bitmap_size.x = 16;
            }
            if (tool_bitmap_size.y == 0)
            {
                tool_bitmap_size.y = 16;
            }

            // manually get the size of toolbars
            height = tool_bitmap_size.GetY() + 16;
            width = 20;

            wxToolBarToolBase* tool;
            wxToolBarToolsList::compatibility_iterator node;

            for (node = tools.GetFirst(); node; node = node->GetNext())
            {
                tool = node->GetData();
                if (tool->IsButton())
                {
                    int tool_width = toolbar->GetToolSize().GetWidth();
                    if (tool_width == 0)
                    {
                        tool_width = 24;
                    }
                    width += tool_width;
                }
                if (tool->IsSeparator())
                {
                    width += 8;
                }
                if (tool->IsControl())
                {
                    wxControl* control = tool->GetControl();
                    int w, h;
                    control->GetSize(&w, &h);
                    width += w;
                }
            }
        }
         else if (wnd->IsKindOf(CLASSINFO(wxAuiToolBar)))
        {
            wxAuiToolBar* toolbar = (wxAuiToolBar*)wnd;
            wxSize s = toolbar->GetMinSize();
            width = s.x + 20;
            height = s.y;
        }
    }

    if (wnd->IsKindOf(CLASSINFO(wxToolBar)))
    {
        prepareToolBar((wxToolBar*)wnd);
    }

    int dock_direction = wxAUI_DOCK_BOTTOM;
    bool floating = false;

    if (site_type & sitetypeDockable)
    {
        if (site_type & dockBottom)
        {
            dock_direction = wxAUI_DOCK_BOTTOM;
        }
         else if (site_type & dockTop)
        {
            dock_direction = wxAUI_DOCK_TOP;
        }
         else if (site_type & dockLeft)
        {
            dock_direction = wxAUI_DOCK_LEFT;
        }
         else if (site_type & dockRight)
        {
            dock_direction = wxAUI_DOCK_RIGHT;
        }
         else if (site_type & dockFloating)
        {
            dock_direction = wxAUI_DOCK_BOTTOM;
            floating = true;
        }

        if (site_type & siteNoResize &&
            !(site_type & dockFloating))
        {
            width += 13;
        }
    }
    
    wxAuiPaneInfo pane;
    
    pane.DefaultPane();

    if (wnd->IsKindOf(CLASSINFO(wxToolBar)))
    {
        pane.ToolbarPane().LeftDockable(false).RightDockable(false);
    }
     else if (wnd->IsKindOf(CLASSINFO(wxAuiToolBar)))
    {
        pane.ToolbarPane().PaneBorder(false).Gripper(false).LeftDockable(false).RightDockable(false);
    }
        
    pane.BestSize(width, height).
         Direction(dock_direction).
         Row(row).
         FloatingSize(width, height+24).
         Resizable((site_type & siteNoResize) ? false:true).
         Show(site_type & siteHidden ? false:true).
         Caption(caption).
         MaximizeButton(false);
    
    if (floating)
        pane.Float();
        
    m_mgr.AddPane(wnd, pane);
}


void MainFrame::dockSite(IDocumentSitePtr site,
                         int dock,
                         int row,
                         int offset,
                         int width,
                         int height)
{
    wxWindow* cont_wnd = site->getContainerWindow();
    m_mgr.DetachPane(cont_wnd);
    dockWindow(cont_wnd, wxEmptyString, dock, row, offset, width, height);
}



void MainFrame::refreshFrameLayout()
{
    m_mgr.Update();
}

IDocumentSitePtr& MainFrame::getLastChild()
{
    return lookupSiteByContainer(m_last_child);
}

IDocumentSitePtr& MainFrame::getActiveChild()
{
    return lookupSiteByContainer(m_active_child);
}

size_t MainFrame::getChildCount()
{
    if (m_doc_sites.size() == 0)
        return 0;

    size_t res = 0;
    
    std::vector<IDocumentSitePtr>::iterator it;
    for (it = m_doc_sites.begin(); it != m_doc_sites.end(); ++it)
    {
        if ((*it)->getSiteType() & sitetypeNormal)
            res++;
    }
    
    return res;
}

IDocumentSitePtr MainFrame::lookupSite(const wxString& site_name)
{
    std::vector<IDocumentSitePtr>::iterator it;
    for (it = m_doc_sites.begin(); it != m_doc_sites.end(); ++it)
    {
        if ((*it)->getName() == site_name)
            return (*it);
    }

    return xcm::null;
}

IDocumentSitePtr MainFrame::lookupSiteById(int id)
{
    std::vector<IDocumentSitePtr>::iterator it;
    for (it = m_doc_sites.begin(); it != m_doc_sites.end(); ++it)
    {
        if ((*it)->getId() == id)
            return (*it);
    }

    return xcm::null;
}

// lookupSiteByContainer returns the in-place active site
// for a givin container.  If the container doesn't exist,
// xcm::null is returned.  In the case that no site is
// in-place active, the most recently created site for a given
// container is returned

IDocumentSitePtr& MainFrame::lookupSiteByContainer(wxWindow* wnd)
{
    IDocumentSitePtr* retval = NULL;
    
    std::vector<IDocumentSitePtr>::iterator it;
    for (it = m_doc_sites.begin(); it != m_doc_sites.end(); ++it)
    {
        if ((*it)->getContainerWindow() == wnd)
        {
            if ((*it)->isInPlaceActive())
                return *it;
                
            retval = &(*it);
        }
    }

    if (!retval)
    {
        m_null_docsite.clear();
        return m_null_docsite;
    }
    
    return *retval;
}

IDocumentSiteEnumPtr MainFrame::getDocumentSites(unsigned int site_type)
{
    xcm::IVectorImpl<IDocumentSitePtr>* vec;
    vec = new xcm::IVectorImpl<IDocumentSitePtr>;

    std::vector<IDocumentSitePtr>::iterator it;
    for (it = m_doc_sites.begin(); it != m_doc_sites.end(); ++it)
    {
        if ((*it)->getSiteType() & site_type)
            vec->append(*it);
    }

    return vec;
}


IDocumentSiteEnumPtr MainFrame::getShownDocumentSites(unsigned int site_type)
{
    xcm::IVectorImpl<IDocumentSitePtr>* vec;
    vec = new xcm::IVectorImpl<IDocumentSitePtr>;

    std::vector<IDocumentSitePtr>::iterator it;
    for (it = m_doc_sites.begin(); it != m_doc_sites.end(); ++it)
    {
        if ((*it)->getSiteType() & site_type)
        {
            ContainerBase* cont = getContainerFromWnd((*it)->getContainerWindow());
            if (cont && (cont->getActiveSite() == *it))
            {
                vec->append(*it);
            }
        }
    }

    return vec;
}


IDocumentSiteEnumPtr MainFrame::getDocumentSitesByContainer(wxWindow* container)
{
    xcm::IVectorImpl<IDocumentSitePtr>* vec;
    vec = new xcm::IVectorImpl<IDocumentSitePtr>;

    std::vector<IDocumentSitePtr>::iterator it;
    for (it = m_doc_sites.begin(); it != m_doc_sites.end(); ++it)
    {
        if ((*it)->getContainerWindow() == container)
            vec->append(*it);
    }

    return vec;
}


void MainFrame::setActiveChild(IDocumentSitePtr child)
{
    wxASSERT(child.isOk());
    
    if (child.isNull())
        return;
        
    if (child == getActiveChild())
        return;
        
    wxWindow* wnd = child->getContainerWindow();
    if (!wnd)
        return;
        
    if (wnd->IsKindOf(CLASSINFO(ChildFrame)))
    {
        ChildFrame* child = (ChildFrame*)wnd;
        child->Activate();
    }

}


bool MainFrame::closeAll(bool force)
{
    wxMDIChildFrame* child = GetActiveChild();
    if (child)
    {
        if (child->IsMaximized())
        {
            child->Maximize(false);
        }
    }


    IDocumentSiteEnumPtr sites = getDocumentSites(sitetypeNormal);
    IDocumentSitePtr doc_site;

    size_t i, site_count = sites->size();

    for (i = 0; i < site_count; ++i)
    {
        doc_site = sites->getItem(i);
        sites->setItem(i, xcm::null);

        // set it as active
        setActiveChild(doc_site);

        // close it

        if (!closeSite(doc_site, force ? closeForce : 0))
        {
            sites.clear();
            doc_site.clear();

            return false;
        }

        
    }

    sites.clear();
    doc_site.clear();

    wxAppHack* app = (wxAppHack*)wxTheApp;
    app->deletePendingObjects();

    return true;
}


bool MainFrame::closeSite(IDocumentSitePtr site, int close_flags)
{
    if (!site)
        return false;
    
    bool force = (close_flags & closeForce) ? true : false;
    bool soft = (close_flags & closeSoft) ? true : false;
    
    
    if (site->isInPlaceActive() && !soft)
    {
        wxWindow* wnd_container = site->getContainerWindow();
        ContainerBase* cont = getContainerFromWnd(wnd_container);
        
        if (!cont->closeContainer(force))
            return false;
    }
     else
    {
        IDocumentPtr doc = site->getDocument();
        if (doc)
        {
            wxWindow* doc_wnd = doc->getDocumentWindow();
            doc.p = NULL;
            if (doc_wnd)
            {
                doc_wnd->Destroy();
            }
        }
    }

    
    std::vector<IDocumentSitePtr>::iterator it;
    it = std::find(m_doc_sites.begin(), m_doc_sites.end(), site);
    if (it != m_doc_sites.end())
    {
        m_doc_sites.erase(it);
    }
    
    return true;
}




void MainFrame::cascade()
{
    Cascade();
    m_tile_lock = TileLock_None;
}

void MainFrame::tileHorizontal()
{
    Tile(wxHORIZONTAL);
    m_tile_lock = TileLock_Horizontal;
}

void MainFrame::tileVertical()
{
    Tile(wxVERTICAL);
    m_tile_lock = TileLock_Vertical;
}

void MainFrame::goNextChild()
{
    ActivateNext();
}

void MainFrame::goPreviousChild()
{
    ActivatePrevious();
}

void MainFrame::onMdiClientSize()
{
    wxMDIChildFrame* child = GetActiveChild();
    if (child == NULL)
        return;
    if (child->IsMaximized())
        return;

    if (m_tile_lock == TileLock_Horizontal)
        tileHorizontal();
    if (m_tile_lock == TileLock_Vertical)
        tileVertical();
}



void MainFrame::onContainerDestructing(wxWindow* window)
{
    IDocumentSiteEnumPtr sites = getDocumentSitesByContainer(window);
    size_t i, size = sites->size();
    
    for (i = 0; i < size; ++i)
    {
        std::vector<IDocumentSitePtr>::iterator it;
        it = std::find(m_doc_sites.begin(), m_doc_sites.end(), sites->getItem(i));
        if (it != m_doc_sites.end())
        {
            m_doc_sites.erase(it);
        }
    }
}


void MainFrame::onContainerNameChanged(ContainerBase* container)
{
    // name of the container changed; update wxAUI's pane name
    
    wxWindow* wnd = container->getWindow();
    wxAuiPaneInfo& pane = m_mgr.GetPane(wnd);
    if (pane.IsOk())
    {
        pane.Name(container->getName());
    }
}



void MainFrame::onModelessContainerDestructing(wxWindow* window)
{

    IDocumentSitePtr s = getActiveChild();
    if (s)
    {
        IDocumentPtr doc = s->getDocument();
        if (doc)
        {
            doc->setDocumentFocus();
        }
    }

}


wxString MainFrame::getUniqueSiteName()
{
    // this code is originally from tango/util.h
    static unsigned int seed = (unsigned)time(NULL);
    srand(++seed);

    int i;
    wxChar temp[33];
    memset((void*)temp, 0, (sizeof(wxChar)*33));
    
    for (i = 0; i < 8; i++)
    {
        temp[i] = wxT('a') + (rand() % 26);
    }

    unsigned int t = time(NULL);
    int div = 308915776;    // 26^6;
    for (i = 8; i < 15; i++)
    {
        temp[i] = wxT('a') + (t/div);
        t -= ((t/div)*div);
        div /= 26;
    }

    return temp;
}


void MainFrame::activateInPlace(IDocumentSitePtr site)
{
    if (!site)
        return;
    
    wxWindow* wnd_container = site->getContainerWindow();
    ContainerBase* container = getContainerFromWnd(wnd_container);
    
    // save the old document window
    wxWindow* old_doc_wnd = container->getDocumentWindow();
    wxWindow* new_doc_wnd = NULL;

    // find out the new document window
    IDocumentPtr doc = site->getDocument();
    if (doc)
    {
        new_doc_wnd = doc->getDocumentWindow();
        doc.clear();
    }
    
    // if the new document window is the same as the old, do nothing
    if (new_doc_wnd == old_doc_wnd)
        return;
        
    
    // deactivate old site
    IDocumentPtr old_doc = container->getDocument();
    if (old_doc)
    {
        old_doc->onSiteDeactivated();
        old_doc.clear();
    }
    
    
    // set our site as the active site in the container    
    container->setActiveSite(site);
    wxSize size = wnd_container->GetClientSize();
    
    // make sure the new active doc is the right size
    new_doc_wnd->SetSize(0, 0, size.GetWidth(), size.GetHeight());
    
    // show the new active doc
    new_doc_wnd->Show();

    // make sure any other document window that
    // used to be active is hidden
    if (old_doc_wnd)
    {
        old_doc_wnd->Show(false);
    }
    
    
    // if the container is a child frame, the ui context
    // needs to be activated (toolbars, menus, etc)
    if (wnd_container->IsKindOf(CLASSINFO(ChildFrame)))
    {
        ChildFrame* child = (ChildFrame*)wnd_container;
        if (m_active_child == child)
        {
            ChildFrame* last_child = m_last_child;
            m_active_child = NULL;
            onChildFrameActivated((ChildFrame*)wnd_container);
            m_last_child = last_child;
        }
    }
}
         

IDocumentSitePtr MainFrame::createSite(wxWindow* wnd_container,
                                       IDocumentPtr document,
                                       bool activate)
{
    wnd_container->Freeze();
    ContainerBase* container = getContainerFromWnd(wnd_container);
    
    // hide currently active site
    if (activate)
    {
        IDocumentSitePtr cur_site = container->getActiveSite();
        if (cur_site)
        {
            IDocumentPtr doc = cur_site->getDocument();
            if (doc)
            {
                wxWindow* wnd = doc->getDocumentWindow();
                if (wnd)
                {
                    wnd->Show(false);
                }
            }
        }
    }
    
    wxString unique_name = getUniqueSiteName();
    m_unique_id++;

    DocumentSite* raw_site = new DocumentSite;
    raw_site->setId(m_unique_id);
    raw_site->setName(unique_name);
    raw_site->setDocument(document);
    raw_site->setContainer(container);
    raw_site->setSiteType(sitetypeNormal);
    IDocumentSitePtr site = raw_site;
    
    
    
    if (!document->initDoc(static_cast<IFrame*>(this),
                           site,
                           wnd_container,
                           m_invisible))
    {
        wnd_container->Thaw();
        return xcm::null;
    }

    m_doc_sites.push_back(raw_site);

    if (activate)
    {
        activateInPlace(site);
    }
     else
    {
        document->getDocumentWindow()->Show(false);
    }
    
    wnd_container->Thaw();
    
    return site;
}
                  
IDocumentSitePtr MainFrame::createSite(IDocumentPtr document,
                                       int site_type,
                                       int x,
                                       int y,
                                       int width,
                                       int height)
{
    wxString unique_name = getUniqueSiteName();
    m_unique_id++;

    int site_x, site_y, site_width, site_height;
    site_x = (x == -1 ? 0 : x);
    site_y = (y == -1 ? 0 : y);

    GetClientWindow()->GetClientSize(&site_width, &site_height);
    if (width != -1)
    {
        site_width = width;
    }
    if (height != -1)
    {
        site_height = height;
    }


    if (site_type & sitetypeNormal)
    {
        wxBusyCursor bc;

        int frame_style = wxDEFAULT_FRAME_STYLE;
        
        #ifdef CFW_USE_TABMDI
            // for tab mdi, this will create the page, but not make it active
            frame_style |= wxMINIMIZE;
            GetClientWindow()->Freeze();
        #endif

        // initially, the window is created off-screen so that no flicker
        // can be seen while the window initializes.  After successful
        // document instantiation, the window is moved to its final
        // starting position

        ChildFrame* container = new ChildFrame(this,
                                           -1,
                                           wxT(""),
                                           wxPoint(4000, 4000),
                                           wxSize(site_width,site_height),
                                           frame_style);
        container->Show(false);

        DocumentSite* raw_site = new DocumentSite;
        raw_site->setId(m_unique_id);
        raw_site->setName(unique_name);
        raw_site->setDocument(document);
        raw_site->setContainer(container);
        raw_site->setSiteType(sitetypeNormal);
        IDocumentSitePtr site = static_cast<IDocumentSite*>(raw_site);
        
        // initialize child
        container->sigFrameActivated.connect(this, &MainFrame::onChildFrameActivated);
        container->sigFrameDeactivated.connect(this, &MainFrame::onChildFrameDeactivated);
        container->sigFrameDestructing.connect(this, &MainFrame::onChildFrameDestructing);
        container->sigFrameMoved.connect(this, &MainFrame::onChildFrameMoved);

        m_last_child = m_active_child;

        // get UI context information
        IUIContextPtr ui_context = static_cast<IUIContext*>(new UIContext);


        if (!document->initDoc(static_cast<IFrame*>(this),
                                            site,
                                            container,
                                            m_invisible))
        {
            raw_site->setDocument(xcm::null);
            m_last_child = NULL;
            m_active_child = NULL;
            m_dispatcher_child->setReceiver(NULL);

            document = xcm::null;
            delete container;
            return xcm::null;
        }


        container->setActiveSite(site);
        

        #ifdef CFW_USE_TABMDI
        if (!(site_type & siteNoInitialActivate))
            container->Activate();

        GetClientWindow()->Thaw();

        // this additional step is necessary to make the display
        // show the tabs correctly; however, it's also causing the
        // siteHidden flag to not work for ChildFrame containers
        ((wxAuiManager&)GetClientWindow()->GetAuiManager()).Update();

        #endif

        m_active_child = m_last_child;


        // determine the total number of children
        // currently in the mdi
        size_t current_child_count;
        {
            IDocumentSiteEnumPtr sites = getDocumentSites(sitetypeNormal);
            current_child_count = sites->size();
        }
        

        m_doc_sites.push_back(site);
        
        container->sigContainerDestructing.connect(this, &MainFrame::onContainerDestructing);
        container->sigContainerNameChanged.connect(this, &MainFrame::onContainerNameChanged);


        // dispatch activate event to child
        
        if (!(site_type & siteNoInitialActivate) || current_child_count == 0)
        {
            wxActivateEvent evt;
            container->onActivate(evt);
        }
        

        #ifndef CFW_USE_TABMDI

        GetClientWindow()->GetClientSize(&site_width, &site_height);
        if (width != -1)
        {
            site_width = width;
        }
        if (height != -1)
        {
            site_height = height;
        }

        // move child to its final position
        container->SetSize(site_x, site_y, site_width, site_height);

        #endif




        if (site_type & siteMaximized)
        {
            #ifdef WIN32
            container->Maximize();
            #endif
        }

        if (!(site_type & siteHidden))
        {
            container->Show(true);
        }

        return static_cast<IDocumentSite*>(site);
    }
     else if (site_type & sitetypeModeless)
    {
        unsigned extra_flags = 0;

        if (site_type & siteCloseButton)
            extra_flags |= wxCLOSE_BOX;
        if (site_type & siteMaximizeButton)
            extra_flags |= wxMAXIMIZE_BOX;
        if (site_type & siteMinimizeButton)
            extra_flags |= wxMINIMIZE_BOX;

        ModelessContainer* container = new ModelessContainer(
                                             this,
                                             -1,
                                             wxPoint(site_x, site_y),
                                             wxSize(site_width, site_height),
                                             extra_flags);
                                                     
        DocumentSite* raw_site = new DocumentSite;
        raw_site->setId(m_unique_id);
        raw_site->setName(unique_name);
        raw_site->setDocument(document);
        raw_site->setSiteType(sitetypeModeless);
        raw_site->setContainer(container);
        IDocumentSitePtr site = static_cast<IDocumentSite*>(raw_site);
        
        
        // get UI context information (this is not used here [yet])
        IUIContextPtr ui_context = static_cast<IUIContext*>(new UIContext);

        if (!document->initDoc(this, site, container, NULL))
        {
            container->Destroy();
            return xcm::null;
        }
        
        container->setDocument(document);
        container->setActiveSite(site);
        
        if (x == -1 || y == -1)
            container->CentreOnScreen();

        container->Layout();

        if (!(site_type & siteHidden))
        {
            container->Show();
        }

        document->setDocumentFocus();


        m_doc_sites.push_back(site);
        
        container->sigContainerDestructing.connect(this, &MainFrame::onContainerDestructing);
        container->sigContainerDestructing.connect(this, &MainFrame::onModelessContainerDestructing);
        container->sigContainerNameChanged.connect(this, &MainFrame::onContainerNameChanged);

        return site;
    }
     else if (site_type & sitetypeMiniModeless)
    {
        MiniModelessContainer* container = new MiniModelessContainer(
                                              this,
                                              -1,
                                              wxPoint(site_x, site_y),
                                              wxSize(site_width, site_height));
        
        
        DocumentSite* raw_site = new DocumentSite;
        raw_site->setId(m_unique_id);
        raw_site->setName(unique_name);
        raw_site->setDocument(document);
        raw_site->setContainer(container);
        raw_site->setSiteType(sitetypeMiniModeless);
        IDocumentSitePtr site = static_cast<IDocumentSite*>(raw_site);
        
        // get UI context information (this is not used here [yet])
        IUIContextPtr ui_context = static_cast<IUIContext*>(new UIContext);

        if (!document->initDoc(this, site, container, NULL))
        {
            container->Destroy();
            return xcm::null;
        }
        
        container->setDocument(document);
        container->setActiveSite(site);

        if (x == -1 || y == -1)
            container->CentreOnScreen();

        container->Layout();

        if (!(site_type & siteHidden))
        {
            container->Show();
        }

        document->setDocumentFocus();


        m_doc_sites.push_back(site);
        
        container->sigContainerDestructing.connect(this, &MainFrame::onContainerDestructing);
        container->sigContainerNameChanged.connect(this, &MainFrame::onContainerNameChanged);

        return site;
    }
     else
    {
        if (site_type & dockFloating)
        {
            site_width = (width == -1 ? 400 : width);
            site_height = (height == -1 ? 150 : height);

            // calculate dimensions if defaults were passed

            wxRect window_rect = GetRect();

            if (x == -1)
                site_x = (window_rect.GetWidth()/2) - (site_width/2) - 5;
                 else
                site_x = x;

            if (y == -1)
                site_y = (window_rect.GetHeight()/2) - (site_height/2) - 20;
                 else
                site_y = y;
        }

        PanelContainer* container = new PanelContainer(
                                             (site_type & dockFloating) ?
                                               m_invisible : this);
        container->m_mgr = &m_mgr;

        DocumentSite* raw_site = new DocumentSite;
        raw_site->setId(m_unique_id);
        raw_site->setName(unique_name);
        raw_site->setDocument(document);
        raw_site->setContainer(container);
        raw_site->setSiteType(sitetypeDockable);
        IDocumentSitePtr site = static_cast<IDocumentSite*>(raw_site);
        
        
        if (!document->initDoc(this, site, container, NULL))
        {
            return xcm::null;
        }

        container->setDocument(document);
        container->setActiveSite(site);

        dockWindow(container,
                   site->getCaption(),
                   site_type,
                   site_x,
                   site_y,
                   site_width,
                   site_height);
                   
        document->setDocumentFocus();


        m_doc_sites.push_back(site);
        container->sigContainerDestructing.connect(this, &MainFrame::onContainerDestructing);
        container->sigContainerNameChanged.connect(this, &MainFrame::onContainerNameChanged);

        return site;
    }

    return xcm::null;
}




// User Interface merging

void MainFrame::doMenuMerge(IUIContextPtr ui)
{
    wxMenuBar* frame_menubar = GetMenuBar();
    if (frame_menubar == NULL)
        return;

    int i;

#ifdef WIN32
    for (i = 0; i < m_menu_merge_count; i++)
    {
        frame_menubar->Remove(m_menu_merge_position);
    }
#else
    // kludge for gtk
    IUIContextInternalPtr ui_internal = m_active_ui;
    for (i = 0; i < m_menu_merge_count; i++)
    {
        ui_internal->getMenuInfo(i).menu = frame_menubar->Remove(m_menu_merge_position);
    }
    ui_internal.clear();
#endif

    m_menu_merge_count = 0;

    if (ui.isOk())
    {
        int menu_count = ui->getMenuCount();
        
        wxString caption;
        wxMenu* menu;

        for (int i = 0; i < menu_count; ++i)
        {
            ui->getMenu(i, menu, caption);
            frame_menubar->Insert(i + m_menu_merge_position, menu, caption);
        }

        m_menu_merge_count = menu_count;
    }
}


void MainFrame::activateUI(IUIContextPtr ui)
{
    if (m_active_ui == ui)
        return;

    int save_tile_lock = m_tile_lock;
    m_tile_lock = TileLock_None;


    IUIContextInternalPtr new_ui_internal = ui;
    IUIContextInternalPtr active_ui_internal = m_active_ui;

    if (ui.isNull())
    {
        doMenuMerge(xcm::null);
        m_active_ui = xcm::null;

        m_tile_lock = save_tile_lock;
        return;
    }


    // first, activate the UI's menu
    doMenuMerge(ui);

    // restore the tile lock setting
    m_tile_lock = save_tile_lock;

    m_active_ui = ui;
}

wxWindow* MainFrame::findWindow(wxWindowID id)
{
    return FindWindow(id);
}

void MainFrame::showWindow(wxWindow* wnd, bool show)
{
    wxAuiPaneInfo& pane = m_mgr.GetPane(wnd);
    if (pane.IsOk())
    {
        pane.Show(show);
        m_mgr.Update();
    }
}


void MainFrame::onChildFrameActivated(ChildFrame* child)
{
    IDocumentPtr doc = child->getDocument();
    if (doc.isNull())
    {
        return;
    }

    // if the frame is already activated, don't do it again
    if (m_active_child == child)
    {
        return;
    }

    m_last_child = m_active_child;
    m_active_child = child;

    m_dispatcher_child->setReceiver(doc->getDocumentWindow());
    activateUI(child->getUIContext());
    fire_onActiveChildChanged(lookupSiteByContainer(child));
    doc->onSiteActivated();
    doc->setDocumentFocus();
}

void MainFrame::onChildFrameDeactivated(ChildFrame* child)
{
    if (m_active_child == child)
    {
        m_dispatcher_child->setReceiver(NULL);
    }

    IDocumentPtr doc = child->getDocument();
    if (doc.isOk())
    {
        doc->onSiteDeactivated();
    }
}

void MainFrame::onChildFrameDestructing(ChildFrame* child)
{
    fire_onSiteClose(lookupSiteByContainer(child));
    
    if (m_active_child == child)
    {
        m_dispatcher_child->setReceiver(NULL);
    }
    
    // for child frames, unregister here, since the site is
    // rendered useless once this destruction sequence is started
    onContainerDestructing(child);

    // if this was the last pane, remove the UI
    if (getChildCount() == 0)
    {
        activateUI(xcm::null);

        m_last_child = NULL;
        m_active_child = NULL;
        fire_onActiveChildChanged(xcm::null);
    }
}


void MainFrame::onChildFrameMoved(ChildFrame*)
{
    // a child frame was moved, so turn of tile locking
    m_tile_lock = TileLock_None;
}



// event handling

void MainFrame::fire_onFrameCommand(int id, int int_param, bool* processed)
{
    sigFrameCommand().fire(id, int_param, processed);
}

void MainFrame::fire_onActiveChildChanged(IDocumentSitePtr doc_site)
{
    sigActiveChildChanged().fire(doc_site);
}

void MainFrame::fire_onFrameClose(wxCloseEvent& evt)
{
    sigFrameClose().fire(evt);
}

void MainFrame::fire_onSiteClose(IDocumentSitePtr site)
{
    sigSiteClose().fire(site);
}

void MainFrame::fire_onFrameSize(wxSizeEvent& size)
{
    sigFrameSize().fire(size);
}

void MainFrame::fire_onFrameDestroy()
{
    sigFrameDestroy().fire();
}

void MainFrame::fire_onFrameBarRightClick()
{
    sigFrameBarRightClick().fire();
}


void MainFrame::dispatchAllEvents()
{
    // this onIdle handler dispatches all pending frame events
    FrameworkEvent* cfw_event;
    while (!m_event_queue.empty())
    {
        cfw_event = m_event_queue.front();
        m_event_queue.pop();

        sigFrameEvent().fire(*cfw_event);

        delete cfw_event;
    }
}


void MainFrame::onIdle(wxIdleEvent& evt)
{
    dispatchAllEvents();
}

void MainFrame::updateWindowEventHandlers()
{
    wxASSERT_MSG(kl::Thread::isMain(), wxT("Call allowed only from main thread"));

#if wxCHECK_VERSION(2,9,0)

    if (!m_dispatcher_child->IsUnlinked())
        this->RemoveEventHandler(m_dispatcher_child);
        
    if (!m_dispatcher_frameevt->IsUnlinked())
        this->RemoveEventHandler(m_dispatcher_frameevt);
        
    std::vector<wxEvtHandler*>::reverse_iterator rit;
    for (rit = m_winevent_handlers.rbegin();
         rit != m_winevent_handlers.rend(); ++rit)
    {
        if (!(*rit)->IsUnlinked())
            this->RemoveEventHandler(*rit);
    }
    
    std::vector<wxEvtHandler*>::iterator it;
    for (it = m_winevent_handlers.begin();
         it != m_winevent_handlers.end(); ++it)
    {
        this->PushEventHandler(*it);
    }       
    
    this->PushEventHandler(m_dispatcher_frameevt);
    this->PushEventHandler(m_dispatcher_child);
    
#else

    SetPreviousHandler(NULL);

    SetNextHandler(m_dispatcher_child);
    m_dispatcher_child->SetPreviousHandler(this);

    wxEvtHandler* last_handler = m_dispatcher_child;

    std::vector<wxEvtHandler*>::iterator it;
    for (it = m_winevent_handlers.begin();
         it != m_winevent_handlers.end(); ++it)
    {
        last_handler->SetNextHandler(*it);
        (*it)->SetPreviousHandler(last_handler);

        last_handler = *it;
    }

    last_handler->SetNextHandler(m_dispatcher_frameevt);
    m_dispatcher_frameevt->SetPreviousHandler(last_handler);
#endif
}

void MainFrame::addWindowEventHandler(wxEvtHandler* event_handler)
{
    wxASSERT_MSG(kl::Thread::isMain(), wxT("Call allowed only from main thread"));

    std::vector<wxEvtHandler*>::iterator it;

    it = std::find(m_winevent_handlers.begin(),
                   m_winevent_handlers.end(),
                   event_handler);

    if (it == m_winevent_handlers.end())
    {
        m_winevent_handlers.push_back(event_handler);
    }
    updateWindowEventHandlers();
}

void MainFrame::removeWindowEventHandler(wxEvtHandler* event_handler)
{
    wxASSERT_MSG(kl::Thread::isMain(), wxT("Call allowed only from main thread"));

    std::vector<wxEvtHandler*>::iterator it;

    it = std::find(m_winevent_handlers.begin(),
                   m_winevent_handlers.end(),
                   event_handler);

    if (it != m_winevent_handlers.end())
    {
        m_winevent_handlers.erase(it);
    }

    updateWindowEventHandlers();
}



class MainFrameEventMarshaler : public wxEvtHandler
{
public:
    
    FrameworkEvent* m_evt;
    std::queue<FrameworkEvent*>* m_queue;
    
    bool ProcessEvent(wxEvent& evt)
    {
        m_queue->push(m_evt);
        if (!wxPendingDelete.Member(this))
            wxPendingDelete.Append(this);
        return true;
    }
};


void MainFrame::postEvent(FrameworkEvent* evt)
{
    if (kl::Thread::isMain())
    {
        m_event_queue.push(evt);
    }
     else
    {
        // Marshal into main thread
        MainFrameEventMarshaler* m = new MainFrameEventMarshaler;
        m->m_evt = evt;
        m->m_queue = &m_event_queue;
        wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED, 10000);
        ::wxPostEvent(m, e);
    }
}


void MainFrame::sendEvent(FrameworkEvent* evt)
{
    sigFrameEvent().fire(*evt);
    delete evt;
}

void MainFrame::propertiesUpdated()
{
}

void MainFrame::statusFieldsUpdated()
{
}

void MainFrame::locationUpdated()
{
}



