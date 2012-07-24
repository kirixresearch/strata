/*!
 *
 * Copyright (c) 2004-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2004-08-11
 *
 */


#include "kcanvas_int.h"
#include "componentline.h"


namespace kcanvas
{


CompLine::CompLine()
{
    // properties
    initProperties(m_properties);
}

CompLine::~CompLine()
{
}

IComponentPtr CompLine::create()
{
    return static_cast<IComponent*>(new CompLine);
}

void CompLine::initProperties(Properties& properties)
{
    Component::initProperties(properties);
    properties.add(PROP_COMP_TYPE, COMP_TYPE_LINE);
}

IComponentPtr CompLine::clone()
{
    CompLine* c = new CompLine;
    c->copy(this);

    return static_cast<IComponent*>(c);
}

void CompLine::copy(IComponentPtr component)
{
    Component::copy(component);
}

void CompLine::render(const wxRect& rect)
{
    ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    canvas->setPen(m_properties);
    canvas->setBrush(m_properties);
    canvas->drawLine(0, 0, getWidth(), getHeight());
}


}; // namespace kcanvas

