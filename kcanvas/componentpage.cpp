/*!
 *
 * Copyright (c) 2004-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2004-08-10
 *
 */


#include "kcanvas_int.h"
#include "componentpage.h"


namespace kcanvas
{


CompPage::CompPage()
{
    // properties
    initProperties(m_properties);
}

CompPage::~CompPage()
{
}

IComponentPtr CompPage::create()
{
    return static_cast<IComponent*>(new CompPage);
}

void CompPage::initProperties(Properties& properties)
{
    Component::initProperties(properties);
    properties.add(PROP_COMP_TYPE, COMP_TYPE_PAGE);
    properties.add(PROP_MARGIN_LEFT, (int)(1.0*CANVAS_MODEL_DPI));
    properties.add(PROP_MARGIN_RIGHT, (int)(1.0*CANVAS_MODEL_DPI));
    properties.add(PROP_MARGIN_TOP, (int)(1.0*CANVAS_MODEL_DPI));
    properties.add(PROP_MARGIN_BOTTOM, (int)(1.0*CANVAS_MODEL_DPI));
}

IComponentPtr CompPage::clone()
{
    CompPage* c = new CompPage;
    c->copy(this);

    return static_cast<IComponent*>(c);
}

void CompPage::copy(IComponentPtr component)
{
    Component::copy(component);
}

void CompPage::render(const wxRect& rect)
{
    ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    // if we're printing, don't render anything
    if (canvas->isPrinting())
        return;

    canvas->setBrush(Brush(COLOR_BLACK));
    canvas->setPen(Pen(COLOR_BLACK));
    canvas->drawRectangle(canvas->dtom_x(2), canvas->dtom_y(2), getWidth(), getHeight());

    canvas->setBrush(Brush(COLOR_WHITE));
    canvas->setPen(Pen(COLOR_BLACK));
    canvas->drawRectangle(0, 0, getWidth(), getHeight());
}

void CompPage::setMargins(int left_margin,
                          int right_margin,
                          int top_margin,
                          int bottom_margin)
{
    setProperty(PROP_MARGIN_LEFT, left_margin);
    setProperty(PROP_MARGIN_RIGHT, right_margin);
    setProperty(PROP_MARGIN_TOP, top_margin);
    setProperty(PROP_MARGIN_BOTTOM, bottom_margin);
}

void CompPage::getMargins(int* left_margin,
                          int* right_margin,
                          int* top_margin,
                          int* bottom_margin) const
{
    PropertyValue value;

    getProperty(PROP_MARGIN_LEFT, value);
    *left_margin = value.getInteger();
    
    getProperty(PROP_MARGIN_RIGHT, value);
    *right_margin = value.getInteger();
    
    getProperty(PROP_MARGIN_TOP, value);
    *top_margin = value.getInteger();
    
    getProperty(PROP_MARGIN_BOTTOM, value);
    *bottom_margin = value.getInteger();
}


}; // namespace kcanvas

