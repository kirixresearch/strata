/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams; Benjamin I. Williams
 * Created:  2006-12-05
 *
 */


#ifndef H_KCANVAS_PROPERTY_H
#define H_KCANVAS_PROPERTY_H


namespace kcanvas
{


// Canvas Property Container Classes

// forward declarations
class Color;
class Pen;
class Brush;
class Font;
class PropertyValue;
class Component;


// property data class definition
class PropertyValue
{
friend class Property;

public:

    PropertyValue();
    virtual ~PropertyValue();

    PropertyValue(const PropertyValue& c);
    PropertyValue(const std::wstring& data);
    PropertyValue(const wxString& data);
    PropertyValue(const Color& data);
    PropertyValue(int data);
    PropertyValue(bool data);

    PropertyValue& operator=(const PropertyValue& data);
    PropertyValue& operator=(const wxString& data);
    PropertyValue& operator=(const Color& data);
    PropertyValue& operator=(int data);
    PropertyValue& operator=(bool data);

    bool operator==(const PropertyValue& c) const;
    bool operator!=(const PropertyValue& c) const;

    int getType() const;
    const wxString& getString() const;
    const Color& getColor() const;
    int getInteger() const;
    bool getBoolean() const;
    
    void setString(const wxString& data);
    void setColor(const Color& data);
    void setInteger(int data);
    void setBoolean(bool data);

    void clear();

private:

    void commonInit();
    
private:

    int m_type;
    wxString* m_string;
    Color* m_color;
    int m_integer;
    bool m_boolean;
};

class Property
{
friend class Properties;
    
public:
    
    Property();
    Property(const wxString& name, int type);
    Property(const Property& c);
    virtual ~Property();

    Property& operator=(const Property& c);
    bool operator==(const Property& c) const;
    bool operator!=(const Property& c) const;

    void setName(const wxString& name);
    const wxString& getName() const;

    void setType(int type);
    int getType() const;

    void setString(const wxString& value);
    const wxString& getString() const;
    
    void setColor(const Color& value);
    const Color& getColor() const;
    
    void setInteger(int value);
    int getInteger() const;
    
    void setBoolean(bool value);
    bool getBoolean() const;

private:

    wxString m_name;
    PropertyValue m_value;
};


// this is like std::hash_map<wxString,PropertyValue>
//WX_DECLARE_STRING_HASH_MAP(PropertyValue, PropertyValueMap);
typedef std::map<wxString, PropertyValue> PropertyValueMap;

class Properties
{
public:

    Properties();
    Properties(const Properties& c);
    virtual ~Properties();

    Properties& operator=(const Properties& c);
    bool operator==(const Properties& c) const;
    bool operator!=(const Properties& c) const;

    Properties& add(const wxString& name);
    Properties& add(const wxString& name, const PropertyValue& value);
    Properties& add(const Properties& props);
    Properties& remove(const wxString& name);
    Properties& remove(const Properties& props);

    Properties& intersect(const Properties& props);
    Properties& subtract(const Properties& props);
    Properties& exclusiveOr(const Properties& props);
    Properties& clear();

    bool set(const wxString& name, const PropertyValue& value);
    bool set(const Properties& props);
    bool get(const wxString& name, PropertyValue& value) const;
    bool get(Properties& props) const;
    
    bool has(const wxString& name) const;
    bool has(const Properties& props) const;

    void list(Properties& props) const;
    void list(std::vector<Property>& properties) const;

    int count() const;
    bool isEmpty() const;

public:

    XCM_IMPLEMENT_SIGNAL1(sigChanged, Properties&)

private:

    void fireSigChanged();
    void stopSignal();
    void allowSignal();

private:

    PropertyValueMap m_properties;
    int m_signalstop_ref;
};


} // namespace kcanvas


#endif

