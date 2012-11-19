/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client Framework
 * Author:   Benjamin I. Williams
 * Created:  2006-07-31
 *
 */


#include "appmain.h"
#include <wx/tokenzr.h>
#include <xcm/xcm.h>
#include "frameworkprops.h"


namespace cfw
{


wxString propName(const wxString& level_1_name,
                  const wxString& level_2_name,
                  const wxString& level_3_name,
                  const wxString& level_4_name)
{
    wxString res = level_1_name;
    
    if (!level_2_name.empty())
    {
        res += wxT("|");
        res += level_2_name;
    }
    
    if (!level_3_name.empty())
    {
        res += wxT("|");
        res += level_3_name;
    }
    
    if (!level_4_name.empty())
    {
        res += wxT("|");
        res += level_4_name;
    }
    
    return res;
}


// PropertyBase class implementation

PropertyBase::PropertyBase()
{
    m_frame = NULL;
}


void PropertyBase::initProperties(IFrame* frame)
{
    m_frame = frame;
    m_frame->sigFrameEvent().connect(this, &PropertyBase::onPBFrameEvent);
}

void PropertyBase::clearProperties()
{
    m_pb_props.clear();
}

void PropertyBase::refreshPropertiesPanel()
{
    wxASSERT_MSG(m_frame, wxT("you need to call initProperties() first"));
    
    m_frame->postEvent(new FrameworkEvent(wxT("cfw.propertiesChanged")));
}


void PropertyBase::onPBFrameEvent(FrameworkEvent& evt)
{
    if (evt.name == wxT("cfw.propertyChanged"))
    {
        // identify if we are the active window or not
        
        IDocumentPtr doc = this;
        
        IDocumentSitePtr active_child = m_frame->getActiveChild();
        IDocumentPtr active_doc;
        if (active_child)
            active_doc = active_child->getDocument();
            
        if (active_doc.isOk() && doc != active_doc)
        {
            // we are not the active doc
            return;
        }
        
        // update our own model
        PropertyValue* data = static_cast<PropertyValue*>(evt.o_param);
        setProperty(evt.s_param, *data);
        
        // let derived class know that a property changed
        onPropertyChanged(evt.s_param);
    }
}




IPropertyInfoEnumPtr PropertyBase::getPropertyEnum()
{
    xcm::IVectorImpl<IPropertyInfoPtr>* vec;
    vec = new xcm::IVectorImpl<IPropertyInfoPtr>;
    
    std::map<wxString,PropEntry>::iterator it;
    for (it = m_pb_props.begin(); it != m_pb_props.end(); ++it)
    {
        PropertyInfo* info = new PropertyInfo;
        info->setName(it->second.m_name);
        info->setType(it->second.m_data.m_type);
        info->setChoiceInfo(it->second.m_choice_strings,
                            it->second.m_choice_ints);
        
        wxArrayString arr;
        wxStringTokenizer tkz(it->second.m_display_name, wxT("|"));
        while (tkz.HasMoreTokens())
        {
            wxString token = tkz.GetNextToken();

            if (token.IsEmpty())
                continue;
                
            arr.Add(token);
        }
        
        info->setDisplayName(arr);
        
        vec->append(static_cast<IPropertyInfo*>(info));
    }
    
    
    return vec;
}

void PropertyBase::defineProperty(const wxString& prop_name,
                                  int type,
                                  const wxString& display_name,
                                  const wxArrayString* choice_strings,
                                  const wxArrayInt* choice_ints)
{
    PropEntry& p = m_pb_props[prop_name];
    p.m_name = prop_name;
    p.m_display_name = display_name;
    p.m_data.m_type = type;
    if (choice_strings)
    {
        p.m_choice_strings = *choice_strings;
        
        if (choice_ints)
            p.m_choice_ints = *choice_ints;
    }
}
    
    
                    
bool PropertyBase::setProperty(const wxString& prop_name, const PropertyValue& value)
{
    if (m_pb_props.find(prop_name) == m_pb_props.end())
        return false;

    PropEntry& p = m_pb_props[prop_name];
    p.m_name = prop_name;
    p.m_data = value;
    
    return true;
}

bool PropertyBase::getProperty(const wxString& prop_name, PropertyValue& value)
{
    if (m_pb_props.find(prop_name) == m_pb_props.end())
        return false;
        
    value = m_pb_props[prop_name].m_data;
    return true;
}





};


