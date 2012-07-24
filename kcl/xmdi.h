/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   Benjamin I. Williams
 * Created:  2003-04-03
 *
 */


#ifndef __KCL_XMDI_H
#define __KCL_XMDI_H


namespace kcl
{

class wxGxMDIParentFrame;
class wxGxMDIChildFrame;
class wxGxMDIChildContainer;
class wxGxMDIClientWindow;



class wxGxMDIParentFrame : public wxFrame
{
public:
    wxGxMDIParentFrame();
    wxGxMDIParentFrame(wxWindow *parent,
                       wxWindowID id,
                       const wxString& title,
                       const wxPoint& pos = wxDefaultPosition,
                       const wxSize& size = wxDefaultSize,
                       long style = wxDEFAULT_FRAME_STYLE | wxVSCROLL | wxHSCROLL,
                       const wxString& name = wxFrameNameStr);

    ~wxGxMDIParentFrame();
    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxString& title,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxDEFAULT_FRAME_STYLE | wxVSCROLL | wxHSCROLL,
                const wxString& name = wxFrameNameStr );

    void SetChildMenuBar(wxGxMDIChildFrame *pChild);

    wxGxMDIChildFrame* GetActiveChild() const;
    inline void SetActiveChild(wxGxMDIChildFrame* pChildFrame);

    wxGxMDIClientWindow *GetClientWindow() const;

    virtual void Cascade();
    virtual void Tile(wxOrientation orient = wxHORIZONTAL);
    virtual void ArrangeIcons();
    virtual void ActivateNext();
    virtual void ActivatePrevious();

protected:
    wxGxMDIClientWindow* m_mdi_client;

    void OnTileHorizontal(wxCommandEvent& event);
    void OnTileVertical(wxCommandEvent& event);

private:
    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(wxGxMDIParentFrame)
};



class WXDLLEXPORT wxGxMDIChildFrame : public wxWindow
{
friend class wxGxMDIParentFrame;

public:
    wxGxMDIChildFrame();
    wxGxMDIChildFrame(wxGxMDIParentFrame *parent,
                      wxWindowID id,
                      const wxString& title,
                      const wxPoint& pos = wxDefaultPosition,
                      const wxSize& size = wxDefaultSize,
                      long style = wxDEFAULT_FRAME_STYLE,
                      const wxString& name = wxFrameNameStr);

    virtual ~wxGxMDIChildFrame();
    bool Create(wxGxMDIParentFrame *parent,
                wxWindowID id,
                const wxString& title,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxDEFAULT_FRAME_STYLE,
                const wxString& name = wxFrameNameStr);

    virtual void SetTitle(const wxString& title);
    virtual wxString GetTitle() const;

    virtual void Activate();
    virtual bool Destroy();

    // no size hints
    virtual void SetSizeHints(int WXUNUSED(minW),
                              int WXUNUSED(minH),
                              int WXUNUSED(maxW) = -1,
                              int WXUNUSED(maxH) = -1,
                              int WXUNUSED(incW) = -1,
                              int WXUNUSED(incH) = -1);

    void SetIcon(const wxIcon &icon);
    void SetIcons(const wxIconBundle &icons);

    virtual void Maximize(bool maximize = TRUE);
    virtual void Restore();
    virtual void Iconize(bool iconize = TRUE);
    virtual bool IsMaximized() const;
    virtual bool IsIconized() const;
    virtual bool ShowFullScreen(bool show, long style);
    virtual bool IsFullScreen() const;

    virtual bool IsTopLevel() const;

    void SetMDIParentFrame(wxGxMDIParentFrame* parentFrame);
    wxGxMDIParentFrame* GetMDIParentFrame() const;

    void DoSetSize(int x, int y,
                   int width, int height,
                   int sizeFlags = wxSIZE_AUTO);

protected:
    wxGxMDIChildContainer* m_container;
    wxGxMDIParentFrame* m_parent_frame;
    wxRect              m_mdi_rect;
    wxString            m_title;

private:
    DECLARE_DYNAMIC_CLASS(wxGxMDIChildFrame)
    DECLARE_EVENT_TABLE()

    void OnSetFocus(wxFocusEvent& event);
    void OnChildFocus(wxChildFocusEvent& event);

    wxGxMDIChildContainer* GetContainer() { return m_container; }

    friend class wxGxMDIClientWindow;
};



class WXDLLEXPORT wxGxMDIClientWindow : public wxControl
{
friend class wxGxMDIParentFrame;
friend class wxGxMDIChildFrame;
friend class wxGxMDIChildContainer;

public:
    wxGxMDIClientWindow();
    wxGxMDIClientWindow(wxGxMDIParentFrame *parent, long style = 0);
    ~wxGxMDIClientWindow();

    wxGxMDIChildContainer* GetActiveChild() const;
    void Tile(wxOrientation orient = wxHORIZONTAL) const;
    void Cascade() const;
    void ActivateNext();
    void ActivatePrevious();

private:

    wxGxMDIChildContainer* m_active_child;

    wxGxMDIChildContainer* CreateContainer();

    void ActivateChild(wxGxMDIChildContainer* child);
    void OnChildDestroy(wxGxMDIChildContainer* child);

    DECLARE_DYNAMIC_CLASS(wxGxMDIClientWindow)
    DECLARE_EVENT_TABLE()
};


};





#endif


