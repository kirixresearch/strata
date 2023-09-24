/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2004-08-11
 *
 */


#ifndef H_KCANVAS_COMPONENTLINE_H
#define H_KCANVAS_COMPONENTLINE_H


namespace kcanvas
{


// Canvas Line Component Classes

class CompLine : public Component
{
    XCM_CLASS_NAME("kcanvas.CompLine")
    XCM_BEGIN_INTERFACE_MAP(CompLine)
        XCM_INTERFACE_CHAIN(Component)
    XCM_END_INTERFACE_MAP()

public:

    CompLine();
    virtual ~CompLine();

    static IComponentPtr create();
    static void initProperties(Properties& properties);

    // IComponent interface
    IComponentPtr clone();
    void copy(IComponentPtr component);
    void render(const wxRect& rect = wxRect());
};


} // namespace kcanvas


#endif

