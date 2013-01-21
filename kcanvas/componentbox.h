/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2004-08-11
 *
 */


#ifndef __KCANVAS_COMPONENTBOX_H
#define __KCANVAS_COMPONENTBOX_H


namespace kcanvas
{


// Canvas Box Component Classes

class CompBox : public Component
{
    XCM_CLASS_NAME("kcanvas.CompBox")
    XCM_BEGIN_INTERFACE_MAP(CompBox)
        XCM_INTERFACE_CHAIN(Component)
    XCM_END_INTERFACE_MAP()

public:

    CompBox();
    virtual ~CompBox();

    static IComponentPtr create();
    static void initProperties(Properties& properties);

    // IComponent interface
    IComponentPtr clone();
    void copy(IComponentPtr component);
    void render(const wxRect& rect = wxRect());
};


}; // namespace kcanvas


#endif

