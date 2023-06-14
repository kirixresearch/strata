/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-12-24
 *
 */


#include "scripthost.h"
#include "xml.h"




namespace scripthost
{


// (CLASS) XmlNode
// Category: XML
// Description: A class that represents an XML data structure, allowing 
//    read/write access to XML files.
// Remarks: The XmlNode class allows XML data to be loaded and saved, as well
//     as accessed as a tree data structure of XmlNodes.

XmlNode::XmlNode()
{
    m_xmldoc = new ref_xmlnode;
    m_xmldoc->ref();
    
    m_node = m_xmldoc;
}

XmlNode::~XmlNode()
{
    m_xmldoc->unref();
}

// (CONSTRUCTOR) XmlNode.constructor
// Description: Creates a new XmlNode.
//
// Syntax: XmlNode()
//
// Remarks: Creates a new XmlNode.

void XmlNode::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

// (METHOD) XmlNode.parse
// Description: Parses the input into an XML data structure.
//
// Syntax: function XmlNode.parse(text : String) : Boolean
//
// Remarks: Parses the input |text| to create an XML data structure.  If
//     |text| is parsed successfully, the function returns true.  If |text|
//     cannot be parsed, the function returns false.
//
// Param(text): The text to parse.
//
// Returns: Returns true if the |text| is successfully parsed into an XML data 
//     structure, and false otherwise.

void XmlNode::parse(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_node || env->getParamCount() < 1)
    {
        retval->setBoolean(false);
        return;
    }
    
    // -- get the xml string; if empty, return false --
    std::wstring xml_text = env->getParam(0)->getString();
    if (xml_text.empty())
    {
        retval->setBoolean(false);
        return;
    }
    
    bool result;
    result = m_node->parse(xml_text);
    retval->setBoolean(result);
}

// (METHOD) XmlNode.load
// Description: Loads an XML file into an XML data structure.
//
// Syntax: function XmlNode.load(input : String) : Boolean
//
// Remarks: Loads an XML file, specfied by |input|, into an XML
//     data structure.  If the |input| file is successfully loaded,
//     the function returns true.  If the |input| file cannot be
//     loaded, the function returns false.
//
// Param(input): The file to load into an XML data structure.
//
// Returns: Returns true if the |input| file is successfully loaded, and
//     false otherwise.

void XmlNode::load(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_node || env->getParamCount() < 1)
    {
        retval->setBoolean(false);
        return;
    }
    
    // -- get the xml string; if empty, return false --
    std::wstring xml_file = env->getParam(0)->getString();
    if (xml_file.empty())
    {
        retval->setBoolean(false);
        return;
    }
    
    bool result;
    result = m_node->load(xml_file);
    retval->setBoolean(result);
}

// (METHOD) XmlNode.save
// Description: Saves an XML data structure to a file.
//
// Syntax: function XmlNode.save(output : String) : Boolean
//
// Remarks: Saves an XML data structure to a file, specified by
//     |output|.  If the XML data is successfully saved to the |output|,
//     file, the function returns true.  If the XML data is not successfully
//     saved to the |output| file, the function returns false.
//
// Param(output): The file to which to save the XML data.
//
// Returns: Returns true if the XML data is successfully saved to the 
//     |output| file, and false otherwise.

void XmlNode::save(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_node || env->getParamCount() < 1)
    {
        retval->setBoolean(false);
        return;
    }
    
    // -- get the xml string; if empty, return false --
    std::wstring xml_file = env->getParam(0)->getString();
    if (xml_file.empty())
    {
        retval->setBoolean(false);
        return;
    }
    
    bool result;
    result = m_node->save(xml_file);
    retval->setBoolean(result);
}

// (METHOD) XmlNode.saveToString
// Description: Saves an XML data structure to a string
//
// Syntax: function XmlNode.saveToString() : String
//
// Remarks: Saves an XML data structure to a string.
//
// Returns: Returns a string representation of an XML data structure.

void XmlNode::saveToString(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: do we want to call it saveToString(), toString(),
    // or something else?

    retval->setNull();
    if (!m_node)
        return;

    // TODO: may want to allow formatting parameters,
    // similar to what getXML() allows
    retval->setString(m_node->getXML());
}

// (METHOD) XmlNode.cloneNode
// Description: Clones the current XmlNode.
//
// Syntax: function XmlNode.cloneNode(deep : Boolean) : XmlNode
//
// Remarks: Clones this XmlNode and returns a reference to the
//     new node. If |deep| is set to true, this node's children
//     will also be cloned; otherwise, only the name, value and
//     properties of this node will be cloned.  By |deep| isn't
//     specified, then by default the children will be cloned.
//
// Param(deep): A flag indicating whether or not to clone this 
//     node's children when cloning this node
// Returns: A reference to the cloned node

void XmlNode::cloneNode(kscript::ExprEnv* env, kscript::Value* retval)
{
    bool deep = true;
    if (env->getParamCount() > 0)
        deep = env->getParam(0)->getBoolean();

    kl::xmlnode& child = m_node->clone(deep);
    if (child.isEmpty())
    {
        retval->setNull();
        return;
    }

    XmlNode* xml_node = XmlNode::createObject(env);
    xml_node->setNode(m_xmldoc, &child);
    retval->setObject(xml_node);
}

// (METHOD) XmlNode.appendChild
// Description: Appends an XmlNode child to the current XmlNode.
//
// Syntax: function XmlNode.appendChild(node_name : String,
//                                      node_content : String) : XmlNode
//
// Remarks: Appends an XmlNode to the current XmlNode, using either the
//     specified XmlNode, or creating a new one with the specified name
//     |node_name| and |node_content|.  If no parameters are specified, 
//     an empty child is created and appended. If |node_name| is specified 
//     but |node_value| isn't specified, a child having |node_name| is 
//     created and appended, but without any content.
//
// Param(node): The XmlNode to append
// Param(node_name): The name of the XmlNode being created
// Param(node_value): The value of the XmlNode being created
// Returns: A reference to the appended node

void XmlNode::appendChild(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_node)
    {
        retval->setNull();
        return;
    }
    
    // if a node is specified, append the node; otherwise,
    // create a new node, set the name and value, and append 
    // the newly created node to the existing node
    kscript::ValueObject* vobj = env->getParam(0)->getObject();
    if (vobj->isKindOf(L"XmlNode"))
    {
        // TODO: right now, we copy the child that we are
        // appending so that the appended child is a copy
        // of the one passed as a parameter; the DOM, however
        // inserts a reference to the passed copy so that
        // the initial object that's specified can be altered
        // and those changes carry over to the appended child;
        // should we move over to the DOM model?

        // append a new child
        kl::xmlnode& child = m_node->addChild();

        // replace the contents of the newly appended child
        // with the internal xml contents of the input object
        XmlNode* xml_node = static_cast<XmlNode*>(vobj);        
        child.copy(*xml_node->m_node, true);

        // return the newly inserted node
        XmlNode* result_xml_node = XmlNode::createObject(env);
        result_xml_node->setNode(m_xmldoc, &child);
        retval->setObject(result_xml_node);
    }
    else
    {
        // get the tag_name string and the contents
        std::wstring node_name, node_value;

        if (env->getParamCount() >= 1)
            node_name = env->getParam(0)->getString();
        if (env->getParamCount() >= 2)
            node_value = env->getParam(1)->getString();
        
        // create a new node and append it
        XmlNode* xml_node = XmlNode::createObject(env);
        kl::xmlnode& new_child = m_node->addChild(node_name);
        new_child.setNodeValue(node_value);
        xml_node->setNode(m_xmldoc, &new_child);
        retval->setObject(xml_node);
    }
}

// (METHOD) XmlNode.removeChild
// Description: Deletes an XmlNode child of the current XmlNode.
//
// Syntax: function XmlNode.removeChild(index : Integer) : Boolean
// Syntax: function XmlNode.removeChild(node : XmlNode) : Boolean
//
// Remarks: Deletes an XmlNode child of the current XmlNode at the
//     specified |index|.  If the child node is successfully deleted,
//     the function returns true.  If the child not is not deleted,
//     the function returns false.
//
// Param(index): The index of the XmlNode child to delete.
//
// Returns: Returns true if the XmlNode child is deleted, and false otherwise.

void XmlNode::removeChild(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_node || env->getParamCount() < 1)
    {
        retval->setBoolean(false);
        return;
    }
    
    if (env->getParam(0)->isNumber())
    {
        int idx = env->getParam(0)->getInteger();
        if (idx < 0 || (size_t)idx >= m_node->getChildCount())
        {
            // bad index -- out of range
            retval->setBoolean(false);
            return;
        }

        retval->setBoolean(m_node->removeChild(idx));
    }
     else if (env->getParam(0)->isObject())
    {
        kscript::ValueObject* vobj = env->getParam(0)->getObject();
        if (vobj->isKindOf(L"XmlNode"))
        {
            XmlNode* xn = (XmlNode*)vobj;
            bool result = m_node->removeChild(*xn->m_node);
            if (result)
            {
                xn->setNode(NULL, NULL);
            }
            retval->setBoolean(result);
        }
    }

    retval->setBoolean(false);
}

// (METHOD) XmlNode.getChild
// Description: Gets an XmlNode child of the current XmlNode.
//
// Syntax: function XmlNode.getChild(index : Integer) : XmlNode
// Syntax: function XmlNode.getChild(name : String) : XmlNode
//
// Remarks: Gets an XmlNode child of the current XmlNode from a specified
//     |index| or |name|.  The function returns the given XmlNode corresponding
//     to the |index| or the |name|.  However, the function returns null if the 
//     |index| is not in the valid range of indexes for the child XmlNodes, or the 
//     |name| is not a valid name of a child.
//
// Param(index): The index of the XmlNode child to return.
// Param(name): The name of the XmlNode child to return.
//
// Returns: The XmlNode child corresponding to the input index or name, or
//     null if no corresponding child exists.

void XmlNode::getChild(kscript::ExprEnv* env, kscript::Value* retval)
{
    // -- set the default return value --
    retval->setNull();

    if (!m_node || env->getParamCount() < 1)
        return;

    // -- create the object to return --
    kscript::Value val;
    
    // -- if the parameter is a number, then get the 
    //    child by the index --
    if (env->getParam(0)->isNumber())
    {   
        int idx = env->getParam(0)->getInteger();
        if (idx < 0 || idx >= (int)m_node->getChildCount())
            return;
        kl::xmlnode& child = m_node->getChild(idx);

        XmlNode* xml_node = XmlNode::createObject(env);
        xml_node->setNode(m_xmldoc, &child);
        retval->setObject(xml_node);
    }
     else
    {
        // -- if the parameter is not a number, then get the 
        //    child by the tag name --
        std::wstring tag_name = env->getParam(0)->getString();
        kl::xmlnode& child = m_node->getChild(tag_name);
        if (child.isEmpty())
            return;

        XmlNode* xml_node = XmlNode::createObject(env);
        xml_node->setNode(m_xmldoc, &child);
        retval->setObject(xml_node);
    }
}


// (METHOD) XmlNode.getChildNodes
// Description: Returns an array of node children
//
// Syntax: function XmlNode.getChildNodes() : Array(XmlNode)
//
// Remarks: Calling getChildNodes() returns an array of XmlNode objects which
//     represent all children of the node.
//
// Returns: An array of XmlNode objects representing all node children

void XmlNode::getChildNodes(kscript::ExprEnv* env, kscript::Value* retval)
{
    // -- set the default return value --
    retval->setArray(env);

    if (!m_node)
        return;
    
    size_t i, count = m_node->getChildCount();
    for (i = 0; i < count; ++i)
    {  
        XmlNode* xml_node = XmlNode::createObject(env);
        kl::xmlnode& child = m_node->getChild(i);
        xml_node->setNode(m_xmldoc, &child);
        
        kscript::Value val;
        val.setObject(xml_node);
        retval->appendMember(&val);
    }
}

// (METHOD) XmlNode.getChildCount
// Description: Gets the number of XmlNode children for the current XmlNode.
//
// Syntax: function XmlNode.getChildCount() : Integer
//
// Remarks: Returns the number of immediate XmlNode children for the current XmlNode.
//
// Returns: The number of XmlNode children for the current XmlNode.
   
void XmlNode::getChildCount(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_node)
    {
        retval->setInteger(0);
        return;
    }
    
    // return the child count
    int result;
    result = (int)m_node->getChildCount();

    retval->setInteger(result);
}


// (METHOD) XmlNode.hasChildNodes
// Description: Determines if the node has any child nodes
//
// Syntax: function XmlNode.hasChildNodes() : Boolean
//
// Returns: True if the node has any child nodes, false otherwise
   
void XmlNode::hasChildNodes(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_node)
    {
        retval->setBoolean(false);
        return;
    }
    
    retval->setBoolean(m_node->hasChildNodes());
}


// (METHOD) XmlNode.setNodeName
// Description: Sets the name for the current XmlNode.
//
// Syntax: function XmlNode.setNodeName(name : String)
//
// Remarks: Sets the name for the current XmlNode from the specified |name|.
//
// Param(name): The new name for the XmlNode child.

void XmlNode::setNodeName(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_node || env->getParamCount() < 1)
        return;
    
    std::wstring node_name = env->getParam(0)->getString();
    m_node->setNodeName(node_name);
}

// (METHOD) XmlNode.getNodeName
// Description: Gets the name for the current XmlNode.
//
// Syntax: function XmlNode.getNodeName() : String
//
// Returns: Returns the name for the current XmlNode.

void XmlNode::getNodeName(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_node)
    {
        retval->setString(L"");
        return;
    }
    
    retval->setString(m_node->getNodeName());
}

// (METHOD) XmlNode.setNodeValue
// Description: Sets the value of the current XmlNode.
//
// Syntax: function XmlNode.setNodeValue(new_value : String)
//
// Remarks: Sets the value of the current XmlNode.
//
// Param(new_value): The new value for the current XmlNode.
    
void XmlNode::setNodeValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_node || env->getParamCount() < 1)
        return;
    
    std::wstring new_value = env->getParam(0)->getString();
    m_node->setNodeValue(new_value);
}

// (METHOD) XmlNode.getNodeValue
// Description: Gets the contents for the current XmlNode.
//
// Syntax: function XmlNode.getNodeValue() : String
//
// Remarks: Returns the value of the current XmlNode.
//
// Returns: Returns the value of the current XmlNode.

void XmlNode::getNodeValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_node)
    {
        retval->setString(L"");
        return;
    }
    
    std::wstring result = m_node->getNodeValue();
    retval->setString(result);
}

// (METHOD) XmlNode.isEmpty
// Description: Indicates whether the current XmlNode is empty or not.
//
// Syntax: function XmlNode.isEmpty() : Boolean
//
// Remarks: Indicates whether the current XmlNode is empty or not.  The function
//     returns true if the current XmlNode is empty, and false otherwise.
//
// Returns: Returns true if the current XmlNode is empty, and false otherwise.

void XmlNode::isEmpty(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_node)
    {
        retval->setBoolean(true);
        return;
    }
    
    retval->setBoolean(m_node->isEmpty());
}


// (METHOD) XmlNode.appendAttribute
// Description: Adds an attribute to the current XmlNode.
//
// Syntax: function XmlNode.appendAttribute(name : String,
//                                          value : String) : Boolean
//
// Remarks: Adds an attribute to the current XmlNode, with the specified
//     |name| and |value|.  If the attribute name and value are not-empty
//     and they are properly set, the function returns true.  However, if 
//     the |name| or |value| are empty, or the name or value are not properly
//     set, the function returns false.
//
// Param(name): The name of the attribute to add.
// Param(value): The value of the attribute to add.
//
// Results: Returns true if the attribute and value are added to the XmlNode,
//     and false otherwise.

void XmlNode::appendAttribute(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_node || env->getParamCount() < 1)
    {
        retval->setBoolean(false);
        return;
    }
    
    std::wstring attr_name;
    std::wstring attr_value;

    if (env->getParamCount() >= 1)
        attr_name = env->getParam(0)->getString();
    if (env->getParamCount() >= 2)
        attr_value = env->getParam(1)->getString();
     
    if (attr_name.empty())
    {
        retval->setBoolean(false);
        return;
    }

    // if the property exists, modify it; otherwise append a new one
    kl::xmlproperty& result = m_node->getProperty(attr_name);
    if (!result.isEmpty())
        result.value = attr_value;
          else
        m_node->appendProperty(attr_name, attr_value);
    
    // if we didn't successfully add the property, return false
    if (m_node->getProperty(attr_name).isEmpty())
    {
        retval->setBoolean(false);
        return;
    }

    retval->setBoolean(true);
    return;
}

// (METHOD) XmlNode.getAttributes
// Description: Gets an array of the node's attributes
//
// Syntax: function XmlNode.getAttributes() : Object
//
// Remarks: Returns an object with named elements corresponding to the
//     attribute names of the node.  Each of these elements, in turn, contain
//     the string value associated with each attribute
//
// Returns: Returns an object with the names and values of the node's attributes

void XmlNode::getAttributes(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setObject(kscript::Object::createObject(env));

    if (!m_node)
        return;
    
    size_t i, count = m_node->getPropertyCount();
    for (i = 0; i < count; ++i)
    {
        kl::xmlproperty& prop = m_node->getProperty(i);
        retval->getMember(prop.name)->setString(prop.value);
    }
}

// (METHOD) XmlNode.hasAttributes
// Description: Determines if the node has any attributes
//
// Syntax: function XmlNode.hasAttributes() : Boolean
//
// Returns: True if the node has any attributes, false otherwise
   
void XmlNode::hasAttributes(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_node)
    {
        retval->setBoolean(false);
        return;
    }
    
    retval->setBoolean(m_node->getPropertyCount() > 0 ? true : false);
}




}; // namespace scripthost
