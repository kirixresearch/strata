/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client Framework
 * Author:   Benjamin I. Williams
 * Created:  2003-05-08
 *
 */


#ifndef __CFW_FRAMEWORK_PRIVATE_H
#define __CFW_FRAMEWORK_PRIVATE_H


#include <queue>
#include "../kcl/kcl.h"
#include <wx/aui/aui.h>


/*
// use our own custom MDI if we are running on GTK
#ifdef __WXGTK__
#define CFW_USE_XMDI
#endif

#ifdef CFW_USE_XMDI
#define wxMDIParentFrame kcl::wxGxMDIParentFrame
#define wxMDIChildFrame kcl::wxGxMDIChildFrame
#define wxMDIClientWindow kcl::wxGxMDIClientWindow
#endif
*/

#define wxMDIParentFrame wxAuiMDIParentFrame
#define wxMDIChildFrame wxAuiMDIChildFrame
#define wxMDIClientWindow wxAuiMDIClientWindow 
#define CFW_USE_XMDI
#define CFW_USE_TABMDI


namespace cfw
{



struct MenuInfo
{
    wxMenu* menu;
    wxString caption;
};

struct PaneInfo
{
    wxWindow* wnd;      // window pointer of the pane's window
    int saved_state;    // fl bar state when the ui is not active
};



xcm_interface IUIContextInternal : public xcm::IObject
{
    XCM_INTERFACE_NAME("cfw.IUIContextInternal");

public:

    virtual int getMenuCount() = 0;
    virtual MenuInfo& getMenuInfo(unsigned int idx) = 0;
};

XCM_DECLARE_SMARTPTR(IUIContextInternal)




class MainFrame;
class FrameCommandDispatch : public wxEvtHandler
{
public:
    
    wxWindow* m_receiver;
    MainFrame* m_frame;

    FrameCommandDispatch();
    void setReceiver(wxWindow* receiver);
    bool ProcessEvent(wxEvent& event);
};



class ContainerBase
{
public:

    ContainerBase()
    {
    }
    
    ~ContainerBase()
    {
    }
    
    virtual void setName(const wxString& name)
    {
        m_container_name = name;
        sigContainerNameChanged(this);
    }
    
    virtual wxString getName()
    {
        return m_container_name;
    }

    virtual void setCaption(const wxString& caption) = 0;
    virtual void setBitmap(const wxBitmap& bitmap) = 0;
    virtual wxWindow* getWindow() = 0;
    virtual void setVisible(bool visible) = 0;
    virtual bool getVisible() = 0;
    virtual void setPosition(int x, int y, int width, int height) = 0;
    virtual void getPosition(int* x, int* y, int* width, int* height) = 0;
    virtual bool closeContainer(bool force = false) = 0;
    
    cfw::IDocumentSitePtr getActiveSite()
    {
        return m_active_site;
    }
    
    void setActiveSite(cfw::IDocumentSitePtr site)
    {
        m_active_site = site;
        
        if (m_active_site)
        {
            setCaption(m_active_site->getCaption());
            setBitmap(m_active_site->getBitmap());
        }
    }
    
    cfw::IDocumentPtr getDocument()
    {
        if (m_active_site)
        {
            return m_active_site->getDocument();
        }
        
        return xcm::null;
    }
    
    wxWindow* getDocumentWindow()
    {
        if (m_active_site)
        {
            cfw::IDocumentPtr doc = m_active_site->getDocument();
            if (doc)
                return doc->getDocumentWindow();
        }
        
        return NULL;
    }
    
public:
    xcm::signal1<wxWindow*> sigContainerDestructing;
    xcm::signal1<ContainerBase*> sigContainerNameChanged;
    
private:

    cfw::IDocumentSitePtr m_active_site;
    wxString m_container_name;
};


class ChildFrame :  public wxMDIChildFrame,
                    public ContainerBase
{
friend class MainFrame;

public:

    ChildFrame(wxMDIParentFrame* parent,
               wxWindowID id = -1,
               const wxString& title = wxEmptyString,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               long style = wxDEFAULT_FRAME_STYLE |
                            wxNO_FULL_REPAINT_ON_RESIZE |
                            wxCLIP_CHILDREN,
               const wxString& name = wxT("frame"));
    
    virtual ~ChildFrame();
    
    // container methods
    void setBitmap(const wxBitmap& bitmap);
    cfw::IUIContextPtr getUIContext();

    unsigned int getSiteType();
    bool getVisible();

    wxWindow* getWindow();
    
    bool closeContainer(bool force = false);

    void setPosition(int x, int y, int width, int size);
    void getPosition(int* x, int* y, int* width, int* size);

    void setCaption(const wxString& caption);
    wxString getCaption();

    void setVisible(bool visible);
    bool closeSite(bool force);
    void setMinSize(int width, int height);
    void setMaxSize(int width, int height);

    // window event handlers
    void onSize(wxSizeEvent& evt);
    void onActivate(wxActivateEvent& evt);
    void onClose(wxCloseEvent& evt);
    void onMove(wxMoveEvent& evt);

protected:

    wxMDIParentFrame* m_parent;
    wxWindow* m_wnd;
    int m_id;
    bool m_closing;

public:
    xcm::signal1<ChildFrame*> sigFrameActivated;
    xcm::signal1<ChildFrame*> sigFrameDeactivated;
    xcm::signal1<ChildFrame*> sigFrameDestructing;
    xcm::signal1<ChildFrame*> sigFrameMoved;

    DECLARE_CLASS(ChildFrame)
    DECLARE_EVENT_TABLE()
};



enum TileLockSetting
{
    TileLock_None = 0,
    TileLock_Horizontal = 1,
    TileLock_Vertical = 2
};


class MainFrame : public wxMDIParentFrame,
                  public cfw::IFrame,
                  public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("cfw.MainFrame")
    XCM_BEGIN_INTERFACE_MAP(MainFrame)
        XCM_INTERFACE_ENTRY(cfw::IFrame)
    XCM_END_INTERFACE_MAP()

    XCM_IMPLEMENT_SIGNAL1(sigFrameEvent, cfw::Event&)
    XCM_IMPLEMENT_SIGNAL1(sigFrameClose, wxCloseEvent&)
    XCM_IMPLEMENT_SIGNAL1(sigFrameSize, wxSizeEvent&)
    XCM_IMPLEMENT_SIGNAL0(sigFrameDestroy)
    XCM_IMPLEMENT_SIGNAL0(sigFrameBarRightClick)
    XCM_IMPLEMENT_SIGNAL3(sigFrameCommand, int, int, bool*)
    XCM_IMPLEMENT_SIGNAL1(sigSiteContextMenu, cfw::IDocumentSitePtr)
    XCM_IMPLEMENT_SIGNAL1(sigActiveChildChanged, cfw::IDocumentSitePtr)
    XCM_IMPLEMENT_SIGNAL1(sigSiteClose, cfw::IDocumentSitePtr)
    
public:

    MainFrame();
    ~MainFrame();

    bool create(wxWindow* parent,
                const wxString& caption,
                int x,
                int y,
                int width,
                int height);

    bool Destroy();

    wxFrame* getFrameWindow();
    wxWindow* getInvisibleWindow();
    wxAuiManager& getAuiManager();
    
    cfw::IStatusBarPtr getStatusBar();

    void setStatusBar(cfw::IStatusBarPtr statusbar);
    void setMenuBar(wxMenuBar* menubar, int menu_merge_position);
    void moveWindow(int x, int y, int width, int height);
    void setCaption(const wxString& caption);
    void setIcon(const wxIcon& icon);
    void setIcons(const wxIconBundle& icons);
    void show(bool visible);

    IUIContextPtr createUIContext(const wxString& name);
    IUIContextPtr lookupUIContext(const wxString& name);

    cfw::IDocumentSitePtr& getLastChild();
    cfw::IDocumentSitePtr& getActiveChild();
    
    cfw::IDocumentSitePtr lookupSite(const wxString& site_name);
    cfw::IDocumentSitePtr lookupSiteById(int id);
    cfw::IDocumentSitePtr& lookupSiteByContainer(wxWindow* wnd);
    cfw::IDocumentSiteEnumPtr getDocumentSites(unsigned int site_type);
    cfw::IDocumentSiteEnumPtr getShownDocumentSites(unsigned int type);
    IDocumentSiteEnumPtr getDocumentSitesByContainer(wxWindow* container);
    void setActiveChild(IDocumentSitePtr child);
    size_t getChildCount();
    
    bool closeSite(cfw::IDocumentSitePtr site, int flags = 0);
    bool closeAll(bool force);

    IDocumentSitePtr createSite(
                  cfw::IDocumentPtr document,
                  int site_type,
                  int x,
                  int y,
                  int width,
                  int height);
          
    IDocumentSitePtr createSite(
                  wxWindow* container,
                  IDocumentPtr document,
                  bool activate);
                                             
    void activateInPlace(
                  IDocumentSitePtr site);
                    
    void dockSite(cfw::IDocumentSitePtr site,
                  int dock,
                  int row,
                  int offset,
                  int width,
                  int height);
                  
    void dockWindow(
                  wxWindow* wnd,
                  const wxString& caption,
                  int dock,
                  int row,
                  int offset,
                  int width,
                  int height);
                  
    void showWindow(wxWindow* wnd, bool show);
    wxWindow* findWindow(wxWindowID id);
    void refreshFrameLayout();

    void cascade();
    void tileHorizontal();
    void tileVertical();
    void goNextChild();
    void goPreviousChild();

    // event methods
    void updateWindowEventHandlers();
    void addWindowEventHandler(wxEvtHandler* event_handler);
    void removeWindowEventHandler(wxEvtHandler* event_handler);

    void postEvent(cfw::Event* evt);
    void sendEvent(Event* evt);
    void dispatchAllEvents();

    void propertiesUpdated();
    void statusFieldsUpdated();
    void locationUpdated();
    
public:

    // events firing
    void fire_onFrameCommand(int id, int int_param, bool* processed);
    void fire_onFrameSize(wxSizeEvent& size);
    void fire_onFrameClose(wxCloseEvent& evt);
    void fire_onFrameDestroy();
    void fire_onFrameBarRightClick();
    void fire_onActiveChildChanged(cfw::IDocumentSitePtr container);
    void fire_onSiteClose(cfw::IDocumentSitePtr site);
    
private:

    // actions
    void onMdiClientSize();
    
    // pane slots
    void onChildFrameActivated(ChildFrame* frame);
    void onChildFrameDeactivated(ChildFrame* frame);
    void onChildFrameDestructing(ChildFrame* frame);
    void onChildFrameMoved(ChildFrame* frame);
    
    void onContainerDestructing(wxWindow* window);
    void onContainerNameChanged(ContainerBase* container);
    
    void onModelessContainerDestructing(wxWindow* window);

    void onIdle(wxIdleEvent& evt);
    void onCloseEvent(wxCloseEvent& evt);
    void onMenuOpen(wxMenuEvent& evt);
    void onSize(wxSizeEvent& evt);
    void onChildRightClick(wxAuiNotebookEvent& evt);
    void onAuiPaneClose(wxAuiManagerEvent& evt);
    
    // menu members

    void prepareToolBar(wxToolBar* toolbar);
    void doMenuMerge(IUIContextPtr ui);
    void activateUI(IUIContextPtr ui);

    // other methods

    wxString getUniqueSiteName();

private:

    wxAuiManager m_mgr;
    
    std::vector<wxEvtHandler*> m_winevent_handlers;
    std::queue<cfw::Event*> m_event_queue;
    std::vector<cfw::IDocumentSitePtr> m_doc_sites;
    std::vector<cfw::IUIContextPtr> m_ui_contexts;

    cfw::IDocumentSitePtr m_null_docsite;

    wxWindow* m_invisible;
    ChildFrame* m_active_child;
    ChildFrame* m_last_child;
    cfw::IUIContextPtr m_active_ui;
    cfw::IStatusBarPtr m_statusbar;
    FrameCommandDispatch* m_dispatcher_child;
    FrameCommandDispatch* m_dispatcher_frameevt;
    int m_unique_id;
    int m_menu_merge_count;
    int m_menu_merge_position;
    int m_tile_lock;

    DECLARE_EVENT_TABLE()
};


};  // namespace cfw


#endif

