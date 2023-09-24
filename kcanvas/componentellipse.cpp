/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2004-08-11
 *
 */


#include "kcanvas_int.h"
#include "componentellipse.h"


namespace kcanvas
{


CompEllipse::CompEllipse()
{
    // properties
    initProperties(m_properties);
}

CompEllipse::~CompEllipse()
{
}

IComponentPtr CompEllipse::create()
{
    return static_cast<IComponent*>(new CompEllipse);
}

void CompEllipse::initProperties(Properties& properties)
{
    Component::initProperties(properties);
    properties.add(PROP_COMP_TYPE, COMP_TYPE_ELLIPSE);
}

IComponentPtr CompEllipse::clone()
{
    CompEllipse* c = new CompEllipse;
    c->copy(this);

    return static_cast<IComponent*>(c);
}

void CompEllipse::copy(IComponentPtr component)
{
    Component::copy(component);
}

void CompEllipse::render(const wxRect& rect)
{
    ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    canvas->setPen(m_properties);
    canvas->setBrush(m_properties);
    canvas->drawEllipse(0, 0, getWidth(), getHeight());
}


} // namespace kcanvas

