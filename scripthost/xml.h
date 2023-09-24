/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-12-24
 *
 */


#ifndef H_SCRIPTHOST_SCRIPTXML_H
#define H_SCRIPTHOST_SCRIPTXML_H



#include <kl/xml.h>



namespace scripthost
{


class ref_xmlnode : public kl::xmlnode
{
public:
    ref_xmlnode() { m_ref_count = 0; }
    void ref() { m_ref_count++; }
    void unref() { if (--m_ref_count == 0) delete this; }
    
private:

    int m_ref_count;
};


class XmlNode : public kscript::ValueObject
{
    BEGIN_KSCRIPT_CLASS("XmlNode", XmlNode)
        KSCRIPT_METHOD("constructor", XmlNode::constructor)
        KSCRIPT_METHOD("parse", XmlNode::parse)
        KSCRIPT_METHOD("load", XmlNode::load)
        KSCRIPT_METHOD("save", XmlNode::save)
        KSCRIPT_METHOD("saveToString", XmlNode::saveToString)
        KSCRIPT_METHOD("cloneNode", XmlNode::cloneNode)        
        KSCRIPT_METHOD("appendChild", XmlNode::appendChild)
        KSCRIPT_METHOD("removeChild", XmlNode::removeChild)
        KSCRIPT_METHOD("getChild", XmlNode::getChild)
        KSCRIPT_METHOD("getChildNodes", XmlNode::getChildNodes)
        KSCRIPT_METHOD("getChildCount", XmlNode::getChildCount)
        KSCRIPT_METHOD("hasChildNodes", XmlNode::hasChildNodes)
        KSCRIPT_METHOD("setNodeName", XmlNode::setNodeName)
        KSCRIPT_METHOD("getNodeName", XmlNode::getNodeName)
        KSCRIPT_METHOD("setNodeValue", XmlNode::setNodeValue)
        KSCRIPT_METHOD("getNodeValue", XmlNode::getNodeValue)
        KSCRIPT_METHOD("isEmpty", XmlNode::isEmpty)
        KSCRIPT_METHOD("appendAttribute", XmlNode::appendAttribute)
        KSCRIPT_METHOD("getAttributes", XmlNode::getAttributes)
        KSCRIPT_METHOD("hasAttributes", XmlNode::hasAttributes)
    END_KSCRIPT_CLASS()

public:

    XmlNode();
    ~XmlNode();
    
    void setNode(ref_xmlnode* doc, kl::xmlnode* node)
    {
        m_xmldoc->unref();
        
        m_xmldoc = doc;
        if (m_xmldoc)
        {
            m_xmldoc->ref();
        }
        
        m_node = node;
    }
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    
    void parse(kscript::ExprEnv* env, kscript::Value* retval);
    
    void setNodeName(kscript::ExprEnv* env, kscript::Value* retval);
    void getNodeName(kscript::ExprEnv* env, kscript::Value* retval);
        
    void setNodeValue(kscript::ExprEnv* env, kscript::Value* retval);
    void getNodeValue(kscript::ExprEnv* env, kscript::Value* retval);
    
    void load(kscript::ExprEnv* env, kscript::Value* retval);
    void save(kscript::ExprEnv* env, kscript::Value* retval);
    void saveToString(kscript::ExprEnv* env, kscript::Value* retval);

    void cloneNode(kscript::ExprEnv* env, kscript::Value* retval);
    
    void appendChild(kscript::ExprEnv* env, kscript::Value* retval);
    void removeChild(kscript::ExprEnv* env, kscript::Value* retval);

    void getChildNodes(kscript::ExprEnv* env, kscript::Value* retval);
    void getChild(kscript::ExprEnv* env, kscript::Value* retval);
    void getChildCount(kscript::ExprEnv* env, kscript::Value* retval);
    void hasChildNodes(kscript::ExprEnv* env, kscript::Value* retval);
    
    void isEmpty(kscript::ExprEnv* env, kscript::Value* retval);

    void appendAttribute(kscript::ExprEnv* env, kscript::Value* retval);
    void getAttributes(kscript::ExprEnv* env, kscript::Value* retval);
    void hasAttributes(kscript::ExprEnv* env, kscript::Value* retval);

public:

    kl::xmlnode* m_node;
    ref_xmlnode* m_xmldoc;
};


} // namespace scripthost


#endif

