/*!
 *
 * Copyright (c) 2005-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client Framework
 * Author:   Aaron L. Williams; Benjamin I. Williams
 * Created:  2005-11-29
 *
 */


//#define USE_PROPERTY_GRID
#ifdef USE_PROPERTY_GRID


#include <wx/wx.h>
#include <wx/wupdlock.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/propdev.h>
#include <wx/propgrid/advprops.h>
#include "propertiespanel.h"
#include "framework.h"


namespace cfw
{

const int PropGridId = 14445;

BEGIN_EVENT_TABLE(PropertiesPanel, wxPanel)
    EVT_PG_CHANGED(PropGridId, PropertiesPanel::onPropertyChange)
END_EVENT_TABLE()


PropertiesPanel::PropertiesPanel()
{

}

PropertiesPanel::~PropertiesPanel()
{

}

bool PropertiesPanel::initDoc(cfw::IFramePtr frame,
                              cfw::IDocumentSitePtr doc_site,
                              wxWindow* docsite_wnd,
                              wxWindow* panesite_wnd)
{
    if (!Create(docsite_wnd, -1,
                wxDefaultPosition,
                wxDefaultSize,
                wxNO_FULL_REPAINT_ON_RESIZE | wxCLIP_CHILDREN | wxNO_BORDER))
    {
        return false;
    }

    doc_site->setCaption(_("Properties"));

    m_pg = new wxPropertyGrid(this,
                              PropGridId,
                              wxDefaultPosition,
                              wxDefaultSize,
                              wxPG_AUTO_SORT |
                              wxPG_SPLITTER_AUTO_CENTER |
                              wxPG_DEFAULT_STYLE |
                              wxNO_BORDER);

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_pg, 1, wxEXPAND);

    SetAutoLayout(true);
    SetSizer(sizer);

    // add frame event handler
    frame->sigActiveChildChanged().connect(this, &PropertiesPanel::onActiveChildChanged);
    frame->sigFrameEvent().connect(this, &PropertiesPanel::onFrameEvent);
    m_frame = frame;
    
    populate();

    return true;
}

wxWindow* PropertiesPanel::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void PropertiesPanel::setDocumentFocus()
{
    m_pg->SetFocus();
}

void PropertiesPanel::onActiveChildChanged(cfw::IDocumentSitePtr doc_site)
{
    populate();
}

void PropertiesPanel::onFrameEvent(cfw::Event& evt)
{
    if (evt.name == wxT("cfw.propertiesChanged"))
    {
        populate();
    }
}

void PropertiesPanel::onPropertyChange(wxPropertyGridEvent& evt)
{
    wxPGId pgid = evt.GetProperty();
    const wxPGValueType* type = evt.GetPropertyValueType();
    wxString type_str = type->GetType();
    
    
    PropertyValue val;
    val.m_name = evt.GetPropertyName();
    
    if (type_str == wxT("string"))
    {
        val.m_type = proptypeString;
        val.m_str = m_pg->GetPropertyValueAsString(pgid);
    }
     else if (type_str == wxT("long"))
    {
        val.m_type = proptypeInteger;
        val.m_int = m_pg->GetPropertyValueAsLong(pgid);
    }
     else if (type_str == wxT("bool"))
    {
        val.m_type = proptypeBoolean;
        val.m_bool = m_pg->GetPropertyValueAsBool(pgid);
    }
     else if (type_str == wxT("wxColour"))
    {
        const wxObject* obj = m_pg->GetPropertyValueAsWxObjectPtr(pgid);
        val.m_type = proptypeColor;
        val.m_color = *(const wxColour*)obj;
    }
    
    cfw::Event* e = new cfw::Event(wxT("cfw.propertyChanged"));
    e->s_param = evt.GetPropertyName();
    e->o_param = &val;
    m_frame->sendEvent(e);
}


void PropertiesPanel::populate()
{
    //wxWindowUpdateLocker win(m_pg);
    
    if (!m_pg)
        return;
    

    if (!m_frame)
    {
        m_last_string = wxT("");
        m_pg->Clear();
        return;
    }
        
    // if there is no active child, delete all present items
    cfw::IDocumentSitePtr doc_site = m_frame->getActiveChild();
    cfw::IPropertiesPtr sp_props;
    if (doc_site)
        sp_props = doc_site->getDocument();
    if (sp_props.isNull())
    {
        m_last_string = wxT("");
        m_pg->Clear();

        return;
    }


    cfw::IPropertyInfoEnumPtr all_props = sp_props->getPropertyEnum();
    size_t i, count = all_props->size();
    
    
    bool must_add_props = false;
    
    // find out if the properties already in the grid are the right ones;  if
    // they aren't, we should clear out the property grid and start fresh.
    // If they are the right ones, just use the existing property entries

    wxString cur_string;
    for (i = 0; i < count; ++i)
    {
        cfw::IPropertyInfoPtr prop = all_props->getItem(i);
        cur_string += prop->getName();
        cur_string += wxString::Format(wxT("(%d)"), prop->getType());
        cur_string += wxT("|");
    }
    
    
    if (cur_string != m_last_string)
    {
        m_last_string = cur_string;
     
        // clear and populate properties
        m_pg->Clear();
        
        for (i = 0; i < count; ++i)
        {
            cfw::IPropertyInfoPtr prop = all_props->getItem(i);
            
            wxString prop_name = prop->getName();
            wxArrayString as = prop->getDisplayName();
            wxArrayString choice_strings;
            wxArrayInt choice_ints;
            prop->getChoiceInfo(choice_strings, choice_ints);
            
            wxPGId cat_id;
            wxPGId item_id;
            wxString item_caption;
            
            if (as.GetCount() > 1)
            {
                cat_id = m_pg->GetPropertyByLabel(as[0]);
                if (!cat_id.IsOk())
                    cat_id = m_pg->Append(wxParentProperty(as[0], wxPG_LABEL));
                    
                item_caption = as[1];
            }
             else if (as.GetCount() > 0)
            {
                cat_id = m_pg->GetRoot();
                
                item_caption = as[0];
            }
             else
            {
                continue;
            }


            PropertyValue val;
            if (!sp_props->getProperty(prop_name, val))
                continue;
            
            switch (prop->getType())
            {
                case cfw::proptypeString:
                {
                    item_id = m_pg->AppendIn(cat_id, wxStringProperty(item_caption,
                                                    wxPG_LABEL,
                                                    val.m_str));
                }
                break;
                    
                case cfw::proptypeInteger:
                {
                    if (choice_strings.GetCount() == 0)
                    {
                        item_id = m_pg->AppendIn(cat_id,
                                        wxIntProperty(item_caption,
                                                    wxPG_LABEL,
                                                    val.m_int));
                    }
                     else
                    {
                        if (choice_ints.GetCount() != choice_strings.GetCount())
                        {
                            item_id = m_pg->AppendIn(cat_id,
                                            wxEnumProperty(item_caption,
                                                        wxPG_LABEL,
                                                        choice_strings));
                        }
                         else
                        {
                            item_id = m_pg->AppendIn(cat_id,
                                            wxEnumProperty(item_caption,
                                                        wxPG_LABEL,
                                                        choice_strings,
                                                        choice_ints));
                        }
                    }
                }
                break;
                    
                case cfw::proptypeBoolean:
                {
                    item_id = m_pg->AppendIn(cat_id, wxBoolProperty(item_caption,
                                                    wxPG_LABEL,
                                                    val.m_bool));
                }
                break;
                
                case cfw::proptypeColor:
                {
                    item_id = m_pg->AppendIn(cat_id, wxColourProperty(item_caption,
                                                    wxPG_LABEL,
                                                    val.m_color));
                }
                break;
            }
            
            
            m_pg->SetPropertyName(item_id, prop_name);
        }
            
        
        return;
    }
    
    
    // update existing properties
    for (i = 0; i < count; ++i)
    {
        cfw::IPropertyInfoPtr prop = all_props->getItem(i);
        wxString prop_name = prop->getName();

        PropertyValue val;
        if (!sp_props->getProperty(prop_name, val))
            continue;
            
        switch (prop->getType())
        {
            case cfw::proptypeString:
                m_pg->SetPropertyValue(prop_name, val.m_str);
                break;
                
            case cfw::proptypeInteger:
                m_pg->SetPropertyValue(prop_name, val.m_int);
                break;
                
            case cfw::proptypeBoolean:
                m_pg->SetPropertyValue(prop_name, val.m_bool);
                break;
            
            case cfw::proptypeColor:
                m_pg->SetPropertyValue(prop_name, val.m_color);
                break;
        }
    }

    
}



}; // namespace cfw

#endif
