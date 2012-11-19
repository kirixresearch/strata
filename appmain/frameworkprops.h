/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client Framework
 * Author:   Benjamin I. Williams
 * Created:  2006-07-31
 *
 */


#ifndef __CFW_FRAMEWORKPROPS_H
#define __CFW_FRAMEWORKPROPS_H


#include <map>
#include "framework.h"


namespace cfw
{


wxString propName(const wxString& level_1_name,
                  const wxString& level_2_name = wxEmptyString,
                  const wxString& level_3_name = wxEmptyString,
                  const wxString& level_4_name = wxEmptyString);


class PropEntry
{
public:
    
    wxString m_display_name;
    wxString m_name;
    wxArrayString m_choice_strings;
    wxArrayInt m_choice_ints;
    PropertyValue m_data;
};




class PropertyInfo : public IPropertyInfo
{
    XCM_CLASS_NAME("cfw.PropertyInfo")
    XCM_BEGIN_INTERFACE_MAP(PropertyInfo)
        XCM_INTERFACE_ENTRY(IPropertyInfo)
    XCM_END_INTERFACE_MAP()
    
public:
    
    PropertyInfo()
    {
        m_type = 0;
    }
    
    void setName(const wxString& name)
    {
        m_name = name;
    }

    wxString getName()
    {
        return m_name;
    }

    void setDisplayName(const wxArrayString& display_name)
    {
        m_display_name = display_name;
    }

    wxArrayString getDisplayName()
    {
        return m_display_name;
    }

    void setType(int type)
    {
        m_type = type;
    }

    int getType()
    {
        return m_type;
    }

    void setChoiceInfo(const wxArrayString& choice_strings,
                       const wxArrayInt& choice_ints)
    {
        m_choice_strings = choice_strings;
        m_choice_ints = choice_ints;
    }
                               
    void getChoiceInfo(wxArrayString& choice_strings,
                       wxArrayInt& choice_ints)
    {
        choice_strings = m_choice_strings;
        choice_ints = m_choice_ints;
    }

private:

    wxString m_name;
    wxArrayString m_display_name;
    int m_type;
    
    wxArrayString m_choice_strings;
    wxArrayInt m_choice_ints;
};


class PropertyBase : public IProperties,
                     public xcm::signal_sink
{
    XCM_CLASS_NAME("cfw.PropertyBase")
    XCM_BEGIN_INTERFACE_MAP(PropertyBase)
        XCM_INTERFACE_ENTRY(IProperties)
    XCM_END_INTERFACE_MAP()

public:

    PropertyBase();

    void initProperties(IFrame* frame);
    void refreshPropertiesPanel();
    
    void defineProperty(const wxString& prop_name,
                        int type,
                        const wxString& display_name,
                        const wxArrayString* choice_strings = NULL,
                        const wxArrayInt* choice_ints = NULL);

    void clearProperties();
    IPropertyInfoEnumPtr getPropertyEnum();
    
                        
    bool setProperty(const wxString& prop_name, const PropertyValue& value);
    bool getProperty(const wxString& prop_name, PropertyValue& value);

    virtual void onPropertyChanged(const wxString& prop_name) { }
    
private:

    void onPBFrameEvent(FrameworkEvent& evt);

private:
    
    std::map<wxString, PropEntry> m_pb_props;
    IFrame* m_frame;
};




};  // namespace cfw


#endif

