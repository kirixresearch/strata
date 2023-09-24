/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2004-08-10
 *
 */


#ifndef H_KCANVAS_COMPONENTPAGE_H
#define H_KCANVAS_COMPONENTPAGE_H


namespace kcanvas
{


// Canvas Page Component Classes

class CompPage : public Component,
                 public ICompPage
{
    XCM_CLASS_NAME("kcanvas.CompPage")
    XCM_BEGIN_INTERFACE_MAP(CompPage)
        XCM_INTERFACE_ENTRY(ICompPage)
        XCM_INTERFACE_CHAIN(Component)
    XCM_END_INTERFACE_MAP()

public:

    CompPage();
    virtual ~CompPage();

    static IComponentPtr create();
    static void initProperties(Properties& properties);

    // IComponent interface
    IComponentPtr clone();
    void copy(IComponentPtr component);
    void render(const wxRect& rect = wxRect());

    // ICompPage interface
    void setMargins(int left_margin,
                    int right_margin,
                    int top_margin,
                    int bottom_margin);
                    
    void getMargins(int* left_margin,
                    int* right_margin,
                    int* top_margin,
                    int* bottom_margin) const;
};


} // namespace kcanvas


#endif

