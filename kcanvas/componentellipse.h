/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2004-08-11
 *
 */


#ifndef H_KCANVAS_COMPONENTELLIPSE_H
#define H_KCANVAS_COMPONENTELLIPSE_H


namespace kcanvas
{


// Canvas Ellipse Component Classes

class CompEllipse : public Component
{
    XCM_CLASS_NAME("kcanvas.CompEllipse")
    XCM_BEGIN_INTERFACE_MAP(CompEllipse)
        XCM_INTERFACE_CHAIN(Component)
    XCM_END_INTERFACE_MAP()

public:

    CompEllipse();
    virtual ~CompEllipse();

    static IComponentPtr create();
    static void initProperties(Properties& properties);

    // IComponent interface
    IComponentPtr clone();
    void copy(IComponentPtr component);
    void render(const wxRect& rect = wxRect());
};


}; // namespace kcanvas


#endif

