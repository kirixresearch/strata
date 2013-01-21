/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2006-12-05
 *
 */


#include "kcanvas_int.h"
#include "property.h"
#include "graphicsobj.h"


namespace kcanvas
{


const wxString myEmptyString;
const Color myEmptyColor;

// this is called by constructors to initialize
// most data members to zero or null
void PropertyValue::commonInit()
{
    m_type = proptypeInvalid;
    m_string = NULL;
    m_color = NULL;
    m_integer = 0;
    m_boolean = false;
}

PropertyValue::PropertyValue()
{
    commonInit();
}

PropertyValue::~PropertyValue()
{
    delete m_string;
    delete m_color;
}

PropertyValue::PropertyValue(const PropertyValue& c)
{
    m_type = c.m_type;
    m_integer = c.m_integer;
    m_boolean = c.m_boolean;
    
    if (c.m_string)
        m_string = new wxString(*c.m_string);
         else
        m_string = NULL;
    
    if (c.m_color)
        m_color = new Color(*c.m_color);
         else
        m_color = NULL;
}

PropertyValue::PropertyValue(const wxString& data)
{
    commonInit();
    m_type = proptypeString;
    m_string = new wxString(data);
}

PropertyValue::PropertyValue(const Color& data)
{
    commonInit();
    m_type = proptypeColor;
    m_color = new Color(data);
}

PropertyValue::PropertyValue(int data)
{
    commonInit();
    m_type = proptypeInteger;
    m_integer = data;
}

PropertyValue::PropertyValue(bool data)
{
    commonInit();
    m_type = proptypeBoolean;
    m_boolean = data;
}

PropertyValue& PropertyValue::operator=(const PropertyValue& c)
{
    if (this == &c)
        return *this;

    if (c.m_type == proptypeString)
    {
        setString(c.getString());
    }
     else if (c.m_type == proptypeColor)
    {
        setColor(c.getColor());
    }
     else
    {
        delete m_string;
        delete m_color;
        memcpy(this, &c, sizeof(PropertyValue));
    }
    
    return *this;
}

PropertyValue& PropertyValue::operator=(const wxString& data)
{
    m_type = proptypeString;
    setString(data);
    return *this;
}

PropertyValue& PropertyValue::operator=(const Color& data)
{
    m_type = proptypeColor;
    setColor(data);
    return *this;
}

PropertyValue& PropertyValue::operator=(int data)
{
    m_type = proptypeInteger;
    setInteger(data);
    return *this;
}

PropertyValue& PropertyValue::operator=(bool data)
{
    m_type = proptypeBoolean;
    setBoolean(data);
    return *this;
}

bool PropertyValue::operator==(const PropertyValue& c) const
{
    if (this == &c)
        return true;

    if (m_type != c.m_type)
        return false;

    switch(m_type)
    {
        default:
        case proptypeInvalid:
            return false;
        case proptypeString:
            if (m_string == c.m_string)
                return true;
            if (m_string && c.m_string && *m_string == *c.m_string)
                return true;
            return false;
        case proptypeColor:
            if (m_color == c.m_color)
                return true;
            if (m_color && c.m_color && *m_color == *c.m_color)
                return true;
            return false;
        case proptypeInteger:
            return (m_integer == c.m_integer);
        case proptypeBoolean:
            return (m_boolean == c.m_boolean);
    }
}

bool PropertyValue::operator!=(const PropertyValue& c) const
{
    return !(*this == c);
}

int PropertyValue::getType() const
{
    return m_type;
}

const wxString& PropertyValue::getString() const
{
    if (!m_string)
        return myEmptyString;

    return *m_string;
}

const Color& PropertyValue::getColor() const
{
    // TODO: fix this; we're returning a reference
    // to a local variable

    if (!m_color)
        return myEmptyColor;

    return *m_color;
}

int PropertyValue::getInteger() const
{
    return m_integer;
}

bool PropertyValue::getBoolean() const
{
    return m_boolean;
}

void PropertyValue::setString(const wxString& data)
{
    if (m_type == proptypeString)
    {
        *m_string = data;
        return;
    }

    m_type = proptypeString;
    
    if (m_string)
        *m_string = data;
         else
        m_string = new wxString(data);
    
    delete m_color;
    m_color = NULL;

    m_integer = 0;
    m_boolean = false;
}

void PropertyValue::setColor(const Color& data)
{
    if (m_type == proptypeColor)
    {
        *m_color = data;
        return;
    }

    m_type = proptypeColor;
    
    if (m_color)
        *m_color = data;
         else
        m_color = new Color(data);
    
    delete m_string;
    m_string = NULL;

    m_integer = 0;
    m_boolean = false;
}

void PropertyValue::setInteger(int data)
{
    if (m_type == proptypeInteger)
    {
        m_integer = data;
        return;
    }
    
    m_type = proptypeInteger;
    
    delete m_string;
    m_string = NULL;
    
    delete m_color;
    m_color = NULL;

    m_integer = data;
    m_boolean = false;
}

void PropertyValue::setBoolean(bool data)
{
    if (m_type == proptypeBoolean)
    {
        m_boolean = data;
        return;
    }
    
    m_type = proptypeBoolean;
 
    delete m_string;
    m_string = NULL;
    
    delete m_color;
    m_color = NULL;

    m_integer = 0;
    m_boolean = data;
}

void PropertyValue::clear()
{
    delete m_string;
    delete m_color;

    commonInit();
}


// Property class implementation
Property::Property()
{
}

Property::Property(const wxString& name, int type)
{
    setName(name);
    setType(type);
}

Property::Property(const Property& c)
{
    m_name = c.m_name;
    m_value = c.m_value;
}

Property::~Property()
{
}

Property& Property::operator=(const Property& c)
{
    if (this == &c)
        return *this;

    m_name = c.m_name;
    m_value = c.m_value;

    return *this;
}

bool Property::operator==(const Property& c) const
{
    if (this == &c)
        return true;

    if (m_name != c.m_name)
        return false;

    return (m_value == c.m_value);
}

bool Property::operator!=(const Property& c) const
{
    return !(*this == c);
}

void Property::setName(const wxString& name)
{
    m_name = name;
}

const wxString& Property::getName() const
{
    return m_name;
}

void Property::setType(int type)
{
    m_value.m_type = type;
}

int Property::getType() const
{
    return m_value.m_type;
}

void Property::setString(const wxString& value)
{
    m_value.setString(value);
}

const wxString& Property::getString() const
{
    return m_value.getString();
}

void Property::setColor(const Color& value)
{
    m_value.setColor(value);
}

const Color& Property::getColor() const
{
    return m_value.getColor();
}

void Property::setInteger(int value)
{
    m_value.setInteger(value);
}

int Property::getInteger() const
{
    return m_value.getInteger();
}

void Property::setBoolean(bool value)
{
    m_value.setBoolean(value);
}

bool Property::getBoolean() const
{
    return m_value.getBoolean();
}


// Properties class implementation
Properties::Properties()
{
    m_signalstop_ref = 0;
}

Properties::Properties(const Properties& c)
{
    m_signalstop_ref = 0;
    m_properties = c.m_properties;
}

Properties::~Properties()
{
}

Properties& Properties::operator=(const Properties& c)
{
    if (this == &c)
        return *this;

    // set the properties        
    m_properties = c.m_properties;
    
    // fire the changed signal and return a reference
    fireSigChanged();
    return *this;
}

bool Properties::operator==(const Properties& c) const
{
    if (this == &c)
        return true;
     
    if (m_properties == c.m_properties)
        return true;
        
    return false;
}

bool Properties::operator!=(const Properties& c) const
{
    return !(*this == c);
}

Properties& Properties::add(const wxString& name)
{
    // note: this function adds a property with the given name and
    // an invalid property to the list of properties

    // stop the signal from firing, add the property, and allow the 
    // signal to fire again
    stopSignal();
    add(name, PropertyValue());
    allowSignal();
    
    // fire the changed signal and return a reference 
    fireSigChanged();
    return *this;
}

Properties& Properties::add(const wxString& name, const PropertyValue& value)
{
    // note: this function adds the property with the given name and
    // value to the list of properties; if the property doesn't already
    // exist, the property is added; if the property already exists,
    // the original property value is replaced with the new value

    // try to find the property
    PropertyValueMap::iterator it = m_properties.find(name);
    if (it == m_properties.end())
    {
        // if we can't find the property, add it
        m_properties[name] = value;
        
        // fire the changed signal and return a reference
        fireSigChanged();
        return *this;
    }

    // we were able to find the property, set the new value
    it->second = value;

    // fire the changed signal and return a reference
    fireSigChanged();
    return *this;
}

Properties& Properties::add(const Properties& props)
{
    // note: this function adds the list of properties to the current
    // set of properties; if the properties are not already there, it
    // adds them to the list of properties; if any of the properties
    // are already there, it overwrites their values with the new values

    // if this is the same set of properties, do nothing
    if (this == &props)
        return *this;

    // stop the signal from firing
    stopSignal();

    // add the properties
    PropertyValueMap::const_iterator it, it_end;

    it_end = props.m_properties.end();
    for (it = props.m_properties.begin(); it != it_end; ++it)
    {
        add(it->first, it->second);
    }
    
    // allow the signal to fire again
    allowSignal();
    
    // fire the changed signal and return a reference 
    fireSigChanged();
    return *this;
}

Properties& Properties::remove(const wxString& name)
{
    // note: this function removes a property with the given name from
    // the list of properties; if the property isn't there, the function
    // does nothing

    // look for the property
    PropertyValueMap::iterator it;
    it = m_properties.find(name);
    
    // if we can't find it, simply return a reference
    if (it == m_properties.end())
        return *this;

    // remove the property
    m_properties.erase(it);
    
    // fire the changed signal and return a reference 
    fireSigChanged();
    return *this;
}

Properties& Properties::remove(const Properties& props)
{
    // note: this function removes the properties with the given
    // names in the input props from the list of properties

    // if this is the same set of properties, clear the properties
    if (this == &props)
        return clear();

    // stop the signal from firing
    bool changed = false;
    stopSignal();

    // get the input properties
    std::vector<Property> input_props;
    props.list(input_props);
    
    // for each of the input properties, see if we have a property by the
    // same name; if so, remove it
    std::vector<Property>::const_iterator it, it_end;
    
    it_end = input_props.end();
    for (it = input_props.begin(); it != it_end; ++it)
    {
        remove(it->getName());
        changed = true;
    }

    // allow the signal to fire again
    allowSignal();

    // if the properties have changed, fire a changed signal
    if (changed)
        fireSigChanged();

    // return a reference
    return *this;
}

Properties& Properties::intersect(const Properties& props)
{
    // note: this function removes the properties that are different 
    // from those listed in the input properties, so that what's left 
    // over are the properties from this container that are the same 
    // as the input properties
 
    // if this is the same set of properties, simply return 
    // a reference
    if (this == &props)
        return *this;
 
    // stop the signal from firing
    bool changed = false;
    stopSignal();
 
    // get the input properties
    std::vector<Property> input_props;
    props.list(input_props);
    
    // for each of the input properties, see if we have the same property;
    // if not, remove it
    std::vector<Property>::const_iterator it, it_end;
    
    it_end = input_props.end();
    for (it = input_props.begin(); it != it_end; ++it)
    {
        // if we have the property and the value is the 
        // same, move on
        PropertyValue value;
        if (get(it->getName(), value) && value == it->m_value)
            continue;

        // if not, remove the property
        remove(it->getName());
        changed = true;
    }

    // allow the signal to fire again
    allowSignal();

    // if the properties have changed, fire a changed signal
    if (changed)
        fireSigChanged();

    // return a reference
    return *this;
}

Properties& Properties::subtract(const Properties& props)
{
    // note: this function removes the properties that are the same as 
    // those listed in the input properties, so that what's left over 
    // are the properties from this container that are different from 
    // the input properties
 
    // if this is the same set of properties, clear the
    // list and return a reference
    if (this == &props)
        return clear();
 
    // stop the signal from firing
    bool changed = false;
    stopSignal();
 
    // get the input properties
    std::vector<Property> input_props;
    props.list(input_props);
    
    // for each of the input properties, see if we have the same property 
    // with the same value; if so, remove it
    std::vector<Property>::const_iterator it, it_end;
    
    it_end = input_props.end();
    for (it = input_props.begin(); it != it_end; ++it)
    {
        PropertyValue value;
        get(it->getName(), value);
        
        if (value == it->m_value)
        {
            remove(it->getName());
            changed = true;
        }
    }

    // allow the signal to fire again
    allowSignal();

    // if the properties have changed, fire a changed signal
    if (changed)
        fireSigChanged();

    // return a reference
    return *this;
}

Properties& Properties::exclusiveOr(const Properties& props)
{
    // note: this function removes the properties in this container that 
    // are the same as those in the input properties and adds the remaining
    // input properties; additional note: we could implement this using the
    // add and the intersect function, but this is less efficient than 
    // calculating the xor directly

    // if this is the same set of properties, clear the
    // list and return a reference
    if (this == &props)
        return clear();

    // stop the signal from firing
    bool changed = false;
    stopSignal();

    // get the input properties
    std::vector<Property> input_props;
    props.list(input_props);
    
    // for each of the input properties, see if we have the same property 
    // with the same value; if so, remove it; otherwise, add it
    std::vector<Property>::const_iterator it, it_end;
    
    it_end = input_props.end();
    for (it = input_props.begin(); it != it_end; ++it)
    {
        PropertyValue value;
        
        wxString name = it->getName();
        get(name, value);
        
        if (value == it->m_value)
            remove(name);
        else
            add(name, value);
            
        changed = true;
    }

    // allow the signal to fire again
    allowSignal();

    // if the properties have changed, fire a changed signal
    if (changed)
        fireSigChanged();
      
    // return a reference
    return *this;
}

Properties& Properties::clear()
{
    // note: this function clears the properties

    // clear the properties
    m_properties.clear();

    // fire the changed signal and return a reference     
    fireSigChanged();
    return *this;
}

bool Properties::set(const wxString& name, const PropertyValue& value)
{
    // note: this function checks to see if a given property exists;
    // if the property exists and is different, it sets the property
    // to the new value and returns true; returns false if the value
    // is unchanged or doesn't exist

    // look for the property
    PropertyValueMap::iterator it;
    it = m_properties.find(name);
    
    // if the property doesn't exist, return false
    if (it == m_properties.end())
        return false;
        
    // property exists; if the value is the same, return false
    if (it->second == value)
        return false;
     
    // the property exists and value is different, so set the value
    it->second = value;
    
    // fire the changed signal and return a reference  
    fireSigChanged();
    return true;
}

bool Properties::set(const Properties& props)
{
    // note: this function checks to see if each of the given properties
    // exist; if a given property exists and is different, it sets the
    // property to the new value; the function returns true if any of
    // the properties are set to a new value; returns false otherwise

    // if this is the same set of properties, return false since all
    // the properties are the same
    if (this == &props)
        return false;

    // stop the signal from firing
    bool changed = false;
    stopSignal();

    // iterate through the input properties; if they exist, set their 
    // value; if any do not exist, set a flag indicating that one or 
    // more of the input properties hasn't been set
    PropertyValueMap::const_iterator it, it_end;

    it_end = props.m_properties.end();
    for (it = props.m_properties.begin(); it != it_end; ++it)
    {
        if (set(it->first, it->second))
            changed = true;
    }

    // allow the signal to fire again
    allowSignal();

    // if any of the input properties are set to
    // a new value, fire a signal
    if (changed)
        fireSigChanged();
    
    return changed;
}

bool Properties::get(const wxString& name, PropertyValue& value) const
{
    // note: this function gets the value of a property with a
    // given name; it returns true if the property exists, and
    // false otherwise

    // see if the property exists; if it doesn't, clear the property and
    // return false; the property is clear to make sure that functions 
    // that test the value returned to see if it's the same as the one 
    // they have behave correctly if the value doesn't exist, and also 
    // to make this function consistent with the other version of get()
    PropertyValueMap::const_iterator it = m_properties.find(name);
    if (it == m_properties.end())
    {
        value.clear();
        return false;
    }

    // set the value and return true
    value = it->second;
    return true;
}

bool Properties::has(const wxString& name) const
{
    // note: this function returns true if a property with
    // the given name exists in this collection; false otherwise

    // if we can't find the property, return false
    PropertyValueMap::const_iterator it = m_properties.find(name);
    if (it == m_properties.end())
        return false;

    // otherwise, return true
    return true;
}

bool Properties::has(const Properties& props) const
{
    // note: this function returns true if all the given properties 
    // exist in this collection; false otherwise

    // if there are no input properties, return false
    if (props.m_properties.empty())
        return false;

    // if there are more input properties than exist in 
    // this collection, some of them aren't contained
    // in this collection, so return false
    if (props.m_properties.size() > m_properties.size())
        return false;

    // iterate through the input properties; if the property
    // doesn't exist in this collection, return false
    PropertyValueMap::const_iterator it, it_end;

    it_end = props.m_properties.end();
    for (it = props.m_properties.begin(); it != it_end; ++it)
    {
        if (m_properties.find(it->first) == m_properties.end())
            return false;
    }

    // all the input properties exist in this collection;
    // return true
    return true;
}

bool Properties::get(Properties& props) const
{
    // note: this function takes a list of input properties and
    // returns the properties and values that are contained by
    // this container that are also in the input list; returns 
    // true if any properties are returned; false otherwise

    // if this is the same set of properties, leave the properties
    // unmodified and return true
    if (this == &props)
        return true;

    // iterate through the list of input properties; if any of them
    // exist in the current set of properties, add them to the list
    // of properties to return
    Properties output;
    bool result = false;

    PropertyValueMap::const_iterator it, it_end;

    it_end = props.m_properties.end();
    for (it = props.m_properties.begin(); it != it_end; ++it)
    {
        // try to get the input property from this set of properties;
        // if we get it, add it to the list of output properties
        PropertyValue value;
        if (get(it->first, value))
        {
            output.add(it->first, value);
            result = true;
            continue;
        }
    }

    // set the input properties to the properties that exist in the
    // current set; if there's any properties to return, return true;
    // otherwise, return false
    props = output;
    return result;
}

void Properties::list(Properties& props) const
{
    // note: this function returns a list of all the properties
    // in the current set of properties
    props.m_properties = m_properties;
}

void Properties::list(std::vector<Property>& properties) const
{
    // note: this function returns an enumeration of the properties

    // make sure the properties are clear
    properties.clear();
    
    // add the properties to the list
    Property prop;
    PropertyValueMap::const_iterator it, it_end;

    it_end = m_properties.end();
    for (it = m_properties.begin(); it != it_end; ++it)
    {
        prop.m_name = it->first;
        prop.m_value = it->second;
        properties.push_back(prop);
    }
}

int Properties::count() const
{ 
    return m_properties.size();
}

bool Properties::isEmpty() const
{
    return m_properties.empty();
}

void Properties::fireSigChanged()
{
    // if the stop-signal reference count is greater than
    // zero, don't fire the signal
    if (m_signalstop_ref > 0)
        return;

    // stop additional signals so that properties altered as
    // a result of the event don't trigger additional signals
    stopSignal();
    
    // fire the event
    sigChanged().fire(*this);
    
    // allow events to be fired again
    allowSignal();
}

void Properties::stopSignal()
{
    // increment the reference count used to stop signals
    m_signalstop_ref++;
}

void Properties::allowSignal()
{
    // decrement the reference count to stop signals
    m_signalstop_ref--;

    // if the reference count is less than zero, set it
    // to zero
    wxASSERT_MSG(m_signalstop_ref >= 0, wxT("allow/stopSignal() mismatch!  Property signal surpression refcount should never go below zero"));
    if (m_signalstop_ref < 0)
        m_signalstop_ref = 0;
}


}; // namespace kcanvas

