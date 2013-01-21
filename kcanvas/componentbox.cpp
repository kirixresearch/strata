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
#include "componentbox.h"


namespace kcanvas
{


CompBox::CompBox()
{
    // properties
    initProperties(m_properties);
}

CompBox::~CompBox()
{
}

IComponentPtr CompBox::create()
{
    return static_cast<IComponent*>(new CompBox);
}

void CompBox::initProperties(Properties& properties)
{
    Component::initProperties(properties);
    properties.add(PROP_COMP_TYPE, COMP_TYPE_BOX);
}

IComponentPtr CompBox::clone()
{
    CompBox* c = new CompBox;
    c->copy(this);

    return static_cast<IComponent*>(c);
}

void CompBox::copy(IComponentPtr component)
{
    Component::copy(component);
}

void CompBox::render(const wxRect& rect)
{
    ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    canvas->setPen(m_properties);
    canvas->setBrush(m_properties);
    canvas->drawRectangle(0, 0, getWidth(), getHeight());
}


}; // namespace kcanvas

