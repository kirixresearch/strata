/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2007-07-11
 *
 */


#ifndef H_KCANVAS_COMPONENTCONTROL_H
#define H_KCANVAS_COMPONENTCONTROL_H


namespace kcanvas
{


// Canvas Control Component Classes

class CompControl : public Component,
                    public IEdit
{
friend class Canvas;

    XCM_CLASS_NAME("kcanvas.CompControl")
    XCM_BEGIN_INTERFACE_MAP(CompControl)
        XCM_INTERFACE_ENTRY(IEdit)    
        XCM_INTERFACE_CHAIN(Component)
    XCM_END_INTERFACE_MAP()

public:

    CompControl(wxControl* control);
    virtual ~CompControl();

    static IComponentPtr create(wxControl* control);
    static void initProperties(Properties& properties);

    // IComponent interface
    IComponentPtr clone();
    void copy(IComponentPtr component);
    void addProperty(const wxString& prop_name, const PropertyValue& value);
    void addProperties(const Properties& properties);
    void removeProperty(const wxString& prop_name);
    bool setProperty(const wxString& prop_name, const PropertyValue& value);
    bool setProperties(const Properties& properties);
    void render(const wxRect& rect = wxRect());

    // IEdit interface
    void beginEdit();
    void endEdit(bool accept);
    bool isEditing() const;

    bool canCut() const;
    bool canCopy() const;
    bool canPaste() const;

    void cut();
    void copy();
    void paste();

    void selectAll();
    void selectNone();

    void clear(bool text = true);

private:

    // event handlers
    void onKey(IEventPtr evt);
    void onMouse(IEventPtr evt);

private:

    void clearCache();

protected:

    wxControl* m_control;

private:

    wxBitmap m_cache_bmp;
    int m_cache_width;
    int m_cache_height;
    
    bool m_editing;
};


}; // namespace kcanvas


#endif

