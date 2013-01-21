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
#include "componentgraph.h"


namespace kcanvas
{


CompGraph::CompGraph()
{
    // properties
    initProperties(m_properties);
}

CompGraph::~CompGraph()
{
}

IComponentPtr CompGraph::create()
{
    return static_cast<IComponent*>(new CompGraph);
}

void CompGraph::initProperties(Properties& properties)
{
    Component::initProperties(properties);
    properties.add(PROP_COMP_TYPE, COMP_TYPE_GRAPH);
}

IComponentPtr CompGraph::clone()
{
    CompGraph* c = new CompGraph;
    c->copy(this);

    return static_cast<IComponent*>(c);
}

void CompGraph::copy(IComponentPtr component)
{
    Component::copy(component);
}

void CompGraph::render(const wxRect& rect)
{
}


}; // namespace kcanvas

