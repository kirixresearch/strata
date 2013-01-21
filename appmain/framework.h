/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-03-06
 *
 */


#ifndef __APP_FRAMEWORK_H
#define __APP_FRAMEWORK_H


#include <wx/aui/aui.h>


class wxAuiToolBar;


xcm_interface IUIContext;
xcm_interface IFrame;
xcm_interface IDocument;
xcm_interface IDocumentSite;
xcm_interface IStatusBarItem;
xcm_interface IStatusBarProvider;
xcm_interface IStatusBar;

XCM_DECLARE_SMARTPTR(IUIContext)
XCM_DECLARE_SMARTPTR(IFrame)
XCM_DECLARE_SMARTPTR(IDocument)
XCM_DECLARE_SMARTPTR(IDocumentSite)
XCM_DECLARE_SMARTPTR(IStatusBarItem)
XCM_DECLARE_SMARTPTR(IStatusBarProvider)
XCM_DECLARE_SMARTPTR(IStatusBar)
XCM_DECLARE_SMARTPTR2(xcm::IVector<IDocumentSitePtr>, IDocumentSiteEnumPtr)
XCM_DECLARE_SMARTPTR2(xcm::IVector<IStatusBarItemPtr>, IStatusBarItemEnumPtr)

const int FirstDocCommandId = 7000;


// -- Frame events used by cfw ------------------------------------------------
//
// "cfw.propertiesChanged"   - fired when one or more document properties
//                             are changed
//
// "cfw.statusFieldsChanged" - fired when one or more status field (for the
//                             status bar are changed
//
// "cfw.locationChanged"     - fired when a document's location/url is changed
//
// ----------------------------------------------------------------------------


struct FrameworkEvent
{
    FrameworkEvent(const wxString& _name,
          unsigned long _l_param = 0,
          const wxString& _s_param = wxEmptyString) :
                name(_name),
                s_param(_s_param),
                l_param(_l_param),
                l_param2(0),
                o_param(NULL) { }

    wxString name;
    wxString s_param;
    wxString s_param2;
    unsigned long l_param;
    unsigned long l_param2;
    wxObject* o_param;
};


enum SiteState
{
    sitetypeNormal =       0x0001,
    sitetypeDockable =     0x0002,
    sitetypeModeless =     0x0004,
    sitetypeMiniModeless = 0x0008,
    sitetypeAll =          0x000f,

    dockTop =              0x0010,
    dockLeft =             0x0020,
    dockBottom =           0x0040,
    dockRight =            0x0080,
    dockFloating =         0x0100,

    siteNoResize =         0x0200,
    siteHidden =           0x0400,
    siteMaximized =        0x0800,
    siteCloseButton =      0x1000,
    siteMaximizeButton =   0x2000,
    siteMinimizeButton =   0x4000,
    siteNoInitialActivate =0x8000
};


enum CloseType
{
    closeForce = 1 << 0,    // force site to close without user intervention
    closeSoft = 1 << 1,     // a 'soft close' only closes the site, trying to
                            // leave the container open for other sites inside
                            // the container.  If the closing site is the last
                            // site in the container, the container itself is
                            // also closed
};


xcm_interface IStatusBarItem : public xcm::IObject
{
    XCM_INTERFACE_NAME("cfw.IStatusBarItem")

public:

    virtual wxString getName() = 0;
    virtual wxString getValue() = 0;
    virtual wxBitmap getBitmap() = 0;
    virtual wxControl* getControl() = 0;
    virtual bool isShown() = 0;
    virtual bool isToggled() = 0;
    
    virtual void setValue(const wxString& value) = 0;
    virtual void setBitmap(const wxBitmap& bitmap) = 0;
    virtual void setProportion(int proportion) = 0;
    virtual void setWidth(int width) = 0;
    virtual void setPadding(int left, int right) = 0;
    virtual void setToolTip(const wxString& tooltip) = 0;
    virtual void setToggleMode(bool mode) = 0;
    virtual void setToggled(bool state) = 0;
    virtual void show(bool show) = 0;
};


xcm_interface IStatusBarProvider : public xcm::IObject
{
    XCM_INTERFACE_NAME("cfw.IStatusBarProvider")

public:

    virtual IStatusBarItemEnumPtr getStatusBarItemEnum() = 0;
    virtual IStatusBarItemPtr addStatusBarItem(const wxString& item_name) = 0;
    virtual IStatusBarItemPtr addStatusBarSeparator(const wxString& item_name) = 0;
};


xcm_interface IStatusBar : public xcm::IObject
{
    XCM_INTERFACE_NAME("cfw.IStatusBar")

public:

    virtual wxAuiToolBar* getStatusBarCtrl() = 0;
    
    virtual IStatusBarItemPtr addItem(
                    const wxString& item_name,
                    int location) = 0;

    virtual IStatusBarItemPtr addControl(
                    wxControl* control,
                    const wxString& item_name,
                    int location) = 0;

    virtual IStatusBarItemPtr addSeparator(
                    const wxString& item_name,
                    int location) = 0;

    virtual IStatusBarItemPtr getItem(
                    const wxString& item_name) = 0;

    virtual void showResizeGripper(bool show) = 0;
    virtual void populate() = 0;
    virtual void refresh()  = 0;
    
    XCM_DECLARE_SIGNAL1(sigItemLeftDblClick, IStatusBarItemPtr)
    XCM_DECLARE_SIGNAL1(sigItemLeftClick, IStatusBarItemPtr)
    XCM_DECLARE_SIGNAL0(sigRefresh)
};


xcm_interface IUIContext : public xcm::IObject
{
    XCM_INTERFACE_NAME("cfw.IUIContext")

public:

    virtual void setName(const wxString& name) = 0;
    virtual wxString getName() = 0;

    virtual void addMenu(wxMenu* menu, const wxString& caption) = 0;
    virtual int getMenuCount() = 0;
    virtual void getMenu(unsigned int idx,
                         wxMenu*& menu,
                         wxString& caption) = 0;

    virtual void setPaneSiteWindow(wxWindow* pane_site) = 0;
    virtual wxWindow* getPaneSiteWindow() = 0;
};


xcm_interface IDocument : public xcm::IObject
{
    XCM_INTERFACE_NAME("cfw.IDocument")

public:

    virtual bool initDoc(IFramePtr frame,
                         IDocumentSitePtr site,
                         wxWindow* docsite_wnd,
                         wxWindow* panesite_wnd) = 0;

    virtual wxString getDocumentTitle() { return wxEmptyString; }
    virtual wxString getDocumentLocation() { return wxEmptyString; }
    virtual wxWindow* getDocumentWindow() = 0;
    virtual void setDocumentFocus() = 0;
    virtual void onSiteDeactivated() { }
    virtual void onSiteActivated() { }
    virtual bool onSiteClosing(bool force) { return true; }
};


xcm_interface IDocumentSite : public xcm::IObject
{
    XCM_INTERFACE_NAME("cfw.IDocumentSite")

public:

    virtual IDocumentPtr getDocument() = 0;
    virtual wxWindow* getContainerWindow() = 0;

    virtual IUIContextPtr getUIContext() { return xcm::null; }
    virtual void setUIContext(IUIContextPtr ui_context) { }

    virtual unsigned int getSiteType() = 0;

    virtual int getId() = 0;

    virtual wxString getName() = 0;
    virtual void setName(const wxString& name) = 0;

    virtual void setCaption(const wxString& caption) = 0;
    virtual wxString getCaption() = 0;
    
    virtual void setBitmap(const wxBitmap& bitmap) = 0;
    virtual wxBitmap getBitmap() = 0;

    virtual bool getVisible() = 0;
    virtual void setVisible(bool visible) = 0;

    virtual void setPosition(int x, int y, int width, int height) = 0;
    virtual void getPosition(int* x, int* y, int* width, int* height) = 0;

    virtual void setMinSize(int width, int height) = 0;
    virtual void setMaxSize(int width, int height) = 0;
    
    virtual bool isInPlaceActive() = 0;
};



xcm_interface IFrame : public xcm::IObject
{
    XCM_INTERFACE_NAME("cfw.IFrame")

public:
    
    virtual bool create(wxWindow* parent,
                        const wxString& caption,
                        int x,
                        int y,
                        int width,
                        int height) = 0;

    virtual wxFrame* getFrameWindow() = 0;
    virtual wxWindow* getInvisibleWindow() = 0;
    virtual wxAuiManager& getAuiManager() = 0;

    virtual void setStatusBar(IStatusBarPtr statusbar) = 0;
    virtual IStatusBarPtr getStatusBar() = 0;
    
    virtual void setMenuBar(wxMenuBar* menubar, int menu_merge_position) = 0;
    virtual void moveWindow(int x, int y, int width, int height) = 0;
    virtual void setCaption(const wxString& caption) = 0;
    virtual void setIcon(const wxIcon& icon) = 0;
    virtual void setIcons(const wxIconBundle& icons) = 0;
    virtual void show(bool visible) = 0;

    virtual IUIContextPtr createUIContext(const wxString& name) = 0;
    virtual IUIContextPtr lookupUIContext(const wxString& name) = 0;

    virtual IDocumentSitePtr& getLastChild() = 0;
    virtual IDocumentSitePtr& getActiveChild() = 0;
    virtual size_t getChildCount() = 0;
    virtual void setActiveChild(IDocumentSitePtr child) = 0;

    virtual IDocumentSitePtr lookupSite(const wxString& site_name) = 0;
    virtual IDocumentSitePtr lookupSiteById(int id) = 0;
    virtual IDocumentSiteEnumPtr getDocumentSites(unsigned int type) = 0;
    virtual IDocumentSiteEnumPtr getShownDocumentSites(unsigned int type) = 0;
    virtual IDocumentSiteEnumPtr getDocumentSitesByContainer(wxWindow* container) = 0;
       
    virtual IDocumentSitePtr createSite(
                    IDocumentPtr document,
                    int site_type,
                    int x,
                    int y,
                    int width,
                    int height) = 0;
                    
    virtual IDocumentSitePtr createSite(
                    wxWindow* container,
                    IDocumentPtr document,
                    bool activate) = 0;
                    
    virtual void activateInPlace(
                    IDocumentSitePtr site) = 0;
                    
    virtual void dockSite(
                    IDocumentSitePtr site,
                    int dock,
                    int row,
                    int offset,
                    int width,
                    int height) = 0;
                    
    virtual void refreshFrameLayout() = 0;
    virtual void showWindow(wxWindow* wnd, bool show) = 0;
    virtual wxWindow* findWindow(wxWindowID id) = 0;
    
    virtual void dockWindow(wxWindow* wnd,
                    const wxString& caption,
                    int site_type,
                    int row,
                    int offset,
                    int width,
                    int height) = 0;

    virtual bool closeAll(bool force) = 0;
    virtual bool closeSite(IDocumentSitePtr site, int flags = 0) = 0;
    
    virtual void cascade() = 0;
    virtual void tileHorizontal() = 0;
    virtual void tileVertical() = 0;
    virtual void goNextChild() = 0;
    virtual void goPreviousChild() = 0;

    virtual void addWindowEventHandler(wxEvtHandler* event_handler) = 0;
    virtual void removeWindowEventHandler(wxEvtHandler* event_handler) = 0;

    virtual void postEvent(FrameworkEvent* evt) = 0;
    virtual void sendEvent(FrameworkEvent* evt) = 0;

    XCM_DECLARE_SIGNAL1(sigFrameEvent, FrameworkEvent&)
    XCM_DECLARE_SIGNAL1(sigFrameClose, wxCloseEvent&)
    XCM_DECLARE_SIGNAL1(sigFrameSize, wxSizeEvent&)
    XCM_DECLARE_SIGNAL0(sigFrameDestroy)
    XCM_DECLARE_SIGNAL0(sigFrameBarRightClick)
    XCM_DECLARE_SIGNAL3(sigFrameCommand, int, int, bool*)
    XCM_DECLARE_SIGNAL1(sigActiveChildChanged, IDocumentSitePtr)
    XCM_DECLARE_SIGNAL1(sigSiteClose, IDocumentSitePtr)
    XCM_DECLARE_SIGNAL1(sigSiteContextMenu, IDocumentSitePtr)
};



IFramePtr createFrameObject();



#endif

