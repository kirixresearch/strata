/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2008-02-28
 *
 */


#ifndef H_APP_SCRIPTWEBDOM_H
#define H_APP_SCRIPTWEBDOM_H


#include "../webconnect/webcontrol.h"


class Node : public kscript::ValueObject
{
public:

    BEGIN_KSCRIPT_CLASS("Node", Node)

        KSCRIPT_METHOD("constructor", Node::constructor)

        // these constants are in all caps because this is the standard
        // when referencing DOM node types -- do not change
        
        KSCRIPT_CONSTANT_INTEGER("ELEMENT_NODE",                 ElementNode)
        KSCRIPT_CONSTANT_INTEGER("ATTRIBUTE_NODE",               AttributeNode)
        KSCRIPT_CONSTANT_INTEGER("TEXT_NODE",                    TextNode)
        KSCRIPT_CONSTANT_INTEGER("CDATA_SECTION_NODE",           CDataSectionNode)
        KSCRIPT_CONSTANT_INTEGER("ENTITY_REFERENCE_NODE",        EntityReferenceNode)
        KSCRIPT_CONSTANT_INTEGER("ENTITY_NODE",                  EntityNode)
        KSCRIPT_CONSTANT_INTEGER("PROCESSING_INSTRUCTION_NODE",  ProcessingInstructionNode)
        KSCRIPT_CONSTANT_INTEGER("COMMENT_NODE",                 CommentNode)
        KSCRIPT_CONSTANT_INTEGER("DOCUMENT_NODE",                DocumentNode)
        KSCRIPT_CONSTANT_INTEGER("DOCUMENT_TYPE_NODE",           DocumentTypeNode)
        KSCRIPT_CONSTANT_INTEGER("DOCUMENT_FRAGMENT_NODE",       DocumentFragmentNode)
        KSCRIPT_CONSTANT_INTEGER("NOTATION_NODE",                NotationNode)

    END_KSCRIPT_CLASS()

public:

    enum
    {
        ElementNode                = 1,
        AttributeNode              = 2,
        TextNode                   = 3,
        CDataSectionNode           = 4,
        EntityReferenceNode        = 5,
        EntityNode                 = 6,
        ProcessingInstructionNode  = 7,
        CommentNode                = 8,
        DocumentNode               = 9,
        DocumentTypeNode           = 10,
        DocumentFragmentNode       = 11,
        NotationNode               = 12
    };
    
public:

    void constructor(kscript::ExprEnv* env, kscript::Value* retval) { }
};


class WebDOMNode : public kscript::ValueObject
{
    BEGIN_KSCRIPT_CLASS("WebDOMNode", WebDOMNode)
        KSCRIPT_GUI_METHOD("constructor", WebDOMNode::constructor)
        KSCRIPT_GUI_METHOD("getOwnerDocument", WebDOMNode::getOwnerDocument)
        KSCRIPT_GUI_METHOD("getNodeName", WebDOMNode::getNodeName)
        KSCRIPT_GUI_METHOD("getNodeType", WebDOMNode::getNodeType)
        KSCRIPT_GUI_METHOD("getNodeValue", WebDOMNode::getNodeValue)
        KSCRIPT_GUI_METHOD("setNodeValue", WebDOMNode::setNodeValue)
        KSCRIPT_GUI_METHOD("getParentNode", WebDOMNode::getParentNode)
        KSCRIPT_GUI_METHOD("getChildNodes", WebDOMNode::getChildNodes)
        KSCRIPT_GUI_METHOD("getFirstChild", WebDOMNode::getFirstChild)
        KSCRIPT_GUI_METHOD("getLastChild", WebDOMNode::getLastChild)
        KSCRIPT_GUI_METHOD("getPreviousSibling", WebDOMNode::getPreviousSibling)
        KSCRIPT_GUI_METHOD("getNextSibling", WebDOMNode::getNextSibling)
        KSCRIPT_GUI_METHOD("insertBefore", WebDOMNode::insertBefore)
        KSCRIPT_GUI_METHOD("replaceChild", WebDOMNode::replaceChild)
        KSCRIPT_GUI_METHOD("removeChild", WebDOMNode::removeChild)
        KSCRIPT_GUI_METHOD("appendChild", WebDOMNode::appendChild)
        KSCRIPT_GUI_METHOD("cloneNode", WebDOMNode::cloneNode)
        KSCRIPT_GUI_METHOD("normalize", WebDOMNode::normalize)
        KSCRIPT_GUI_METHOD("hasChildNodes", WebDOMNode::hasChildNodes)
        KSCRIPT_GUI_METHOD("hasAttributes", WebDOMNode::hasAttributes)
        KSCRIPT_GUI_METHOD("getPrefix", WebDOMNode::getPrefix)
        KSCRIPT_GUI_METHOD("setPrefix", WebDOMNode::setPrefix)
        KSCRIPT_GUI_METHOD("getNamespaceURI", WebDOMNode::getNamespaceURI)
        KSCRIPT_GUI_METHOD("getLocalName", WebDOMNode::getLocalName)
        KSCRIPT_GUI_METHOD("addEventListener", WebDOMNode::addEventListener)
    END_KSCRIPT_CLASS()

public:
    
    WebDOMNode();
    ~WebDOMNode();
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void getOwnerDocument(kscript::ExprEnv* env, kscript::Value* retval);
    void getNodeName(kscript::ExprEnv* env, kscript::Value* retval);
    void getNodeType(kscript::ExprEnv* env, kscript::Value* retval);
    void getNodeValue(kscript::ExprEnv* env, kscript::Value* retval);
    void setNodeValue(kscript::ExprEnv* env, kscript::Value* retval);
    void getParentNode(kscript::ExprEnv* env, kscript::Value* retval);
    void getChildNodes(kscript::ExprEnv* env, kscript::Value* retval);
    void getFirstChild(kscript::ExprEnv* env, kscript::Value* retval);
    void getLastChild(kscript::ExprEnv* env, kscript::Value* retval);
    void getPreviousSibling(kscript::ExprEnv* env, kscript::Value* retval);
    void getNextSibling(kscript::ExprEnv* env, kscript::Value* retval);
    void insertBefore(kscript::ExprEnv* env, kscript::Value* retval);
    void replaceChild(kscript::ExprEnv* env, kscript::Value* retval);
    void removeChild(kscript::ExprEnv* env, kscript::Value* retval);
    void appendChild(kscript::ExprEnv* env, kscript::Value* retval);
    void cloneNode(kscript::ExprEnv* env, kscript::Value* retval);
    void normalize(kscript::ExprEnv* env, kscript::Value* retval);
    void hasChildNodes(kscript::ExprEnv* env, kscript::Value* retval);
    void hasAttributes(kscript::ExprEnv* env, kscript::Value* retval);
    void getPrefix(kscript::ExprEnv* env, kscript::Value* retval);
    void setPrefix(kscript::ExprEnv* env, kscript::Value* retval);
    void getNamespaceURI(kscript::ExprEnv* env, kscript::Value* retval);
    void getLocalName(kscript::ExprEnv* env, kscript::Value* retval);
    void addEventListener(kscript::ExprEnv* env, kscript::Value* retval);

public:

    wxDOMNode m_node;
};


class WebDOMText : public WebDOMNode
{
    BEGIN_KSCRIPT_DERIVED_CLASS("WebDOMText", WebDOMText, WebDOMNode)
        KSCRIPT_GUI_METHOD("constructor", WebDOMText::constructor)
        KSCRIPT_GUI_METHOD("setData", WebDOMText::setData)
        KSCRIPT_GUI_METHOD("getData", WebDOMText::getData)
    END_KSCRIPT_CLASS()

public:

    WebDOMText();
    ~WebDOMText();
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setData(kscript::ExprEnv* env, kscript::Value* retval);
    void getData(kscript::ExprEnv* env, kscript::Value* retval);
};


class WebDOMAttr : public WebDOMNode
{
    BEGIN_KSCRIPT_DERIVED_CLASS("WebDOMAttr", WebDOMAttr, WebDOMNode)
        KSCRIPT_GUI_METHOD("constructor", WebDOMAttr::constructor)
        KSCRIPT_GUI_METHOD("getName", WebDOMAttr::getName)
        KSCRIPT_GUI_METHOD("getSpecified", WebDOMAttr::getSpecified)
        KSCRIPT_GUI_METHOD("getValue", WebDOMAttr::getValue)
        KSCRIPT_GUI_METHOD("getOwnerElement", WebDOMAttr::getOwnerElement)
    END_KSCRIPT_CLASS()

public:

    WebDOMAttr();
    ~WebDOMAttr();
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void getName(kscript::ExprEnv* env, kscript::Value* retval);
    void getSpecified(kscript::ExprEnv* env, kscript::Value* retval);
    void getValue(kscript::ExprEnv* env, kscript::Value* retval);
    void getOwnerElement(kscript::ExprEnv* env, kscript::Value* retval);
};


class WebDOMElement : public WebDOMNode
{
    BEGIN_KSCRIPT_DERIVED_CLASS("WebDOMElement", WebDOMElement, WebDOMNode)
        KSCRIPT_GUI_METHOD("constructor", WebDOMElement::constructor)
        KSCRIPT_GUI_METHOD("getTagName", WebDOMElement::getTagName)
        KSCRIPT_GUI_METHOD("getAttributes", WebDOMElement::getAttributes)
        KSCRIPT_GUI_METHOD("getElementsByTagName", WebDOMElement::getElementsByTagName)
        KSCRIPT_GUI_METHOD("getAttribute", WebDOMElement::getAttribute)
        KSCRIPT_GUI_METHOD("setAttribute", WebDOMElement::setAttribute)
        KSCRIPT_GUI_METHOD("removeAttribute", WebDOMElement::removeAttribute)
        KSCRIPT_GUI_METHOD("getAttributeNode", WebDOMElement::getAttributeNode)
        KSCRIPT_GUI_METHOD("setAttributeNode", WebDOMElement::setAttributeNode)
        KSCRIPT_GUI_METHOD("removeAttributeNode", WebDOMElement::removeAttributeNode)
        KSCRIPT_GUI_METHOD("hasAttribute", WebDOMElement::hasAttribute)
    END_KSCRIPT_CLASS()

public:

    WebDOMElement();
    ~WebDOMElement();
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void getTagName(kscript::ExprEnv* env, kscript::Value* retval);
    void getAttributes(kscript::ExprEnv* env, kscript::Value* retval);
    void getElementsByTagName(kscript::ExprEnv* env, kscript::Value* retval);
    void getAttribute(kscript::ExprEnv* env, kscript::Value* retval);
    void setAttribute(kscript::ExprEnv* env, kscript::Value* retval);
    void removeAttribute(kscript::ExprEnv* env, kscript::Value* retval);
    void getAttributeNode(kscript::ExprEnv* env, kscript::Value* retval);
    void setAttributeNode(kscript::ExprEnv* env, kscript::Value* retval);
    void removeAttributeNode(kscript::ExprEnv* env, kscript::Value* retval);
    void hasAttribute(kscript::ExprEnv* env, kscript::Value* retval);
};


class WebDOMDocument : public WebDOMNode
{
    BEGIN_KSCRIPT_DERIVED_CLASS("WebDOMDocument", WebDOMDocument, WebDOMNode)
        KSCRIPT_GUI_METHOD("constructor", WebDOMDocument::constructor)
        KSCRIPT_GUI_METHOD("getDocumentElement", WebDOMDocument::getDocumentElement)
        KSCRIPT_GUI_METHOD("getElementById", WebDOMDocument::getElementById)
        KSCRIPT_GUI_METHOD("getElementsByTagName", WebDOMDocument::getElementsByTagName)
        KSCRIPT_GUI_METHOD("importNode", WebDOMDocument::importNode)
        KSCRIPT_GUI_METHOD("createElement", WebDOMDocument::createElement)
        KSCRIPT_GUI_METHOD("createAttribute", WebDOMDocument::createAttribute)
        KSCRIPT_GUI_METHOD("createTextNode", WebDOMDocument::createTextNode)
    END_KSCRIPT_CLASS()

public:

    WebDOMDocument();
    ~WebDOMDocument();
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void getDocumentElement(kscript::ExprEnv* env, kscript::Value* retval);
    void getElementById(kscript::ExprEnv* env, kscript::Value* retval);
    void getElementsByTagName(kscript::ExprEnv* env, kscript::Value* retval);
    void importNode(kscript::ExprEnv* env, kscript::Value* retval);
    void createElement(kscript::ExprEnv* env, kscript::Value* retval);
    void createAttribute(kscript::ExprEnv* env, kscript::Value* retval);
    void createTextNode(kscript::ExprEnv* env, kscript::Value* retval);
};


class WebDOMHTMLElement : public WebDOMElement
{
    BEGIN_KSCRIPT_DERIVED_CLASS("WebDOMHTMLElement", WebDOMHTMLElement, WebDOMElement)
        KSCRIPT_GUI_METHOD("constructor", WebDOMHTMLElement::constructor)
        KSCRIPT_GUI_METHOD("getId", WebDOMHTMLElement::getId)
        KSCRIPT_GUI_METHOD("setId", WebDOMHTMLElement::setId)
        KSCRIPT_GUI_METHOD("getTitle", WebDOMHTMLElement::getTitle)
        KSCRIPT_GUI_METHOD("setTitle", WebDOMHTMLElement::setTitle)
        KSCRIPT_GUI_METHOD("getLang", WebDOMHTMLElement::getLang)
        KSCRIPT_GUI_METHOD("setLang", WebDOMHTMLElement::setLang)
        KSCRIPT_GUI_METHOD("getDir", WebDOMHTMLElement::getDir)
        KSCRIPT_GUI_METHOD("setDir", WebDOMHTMLElement::setDir)
        KSCRIPT_GUI_METHOD("getClassName", WebDOMHTMLElement::getClassName)
        KSCRIPT_GUI_METHOD("setClassName", WebDOMHTMLElement::setClassName)
        KSCRIPT_GUI_METHOD("getValue", WebDOMHTMLElement::getValue)
        KSCRIPT_GUI_METHOD("setValue", WebDOMHTMLElement::setValue)
        KSCRIPT_GUI_METHOD("getTabIndex", WebDOMHTMLElement::getTabIndex)
        KSCRIPT_GUI_METHOD("setTabIndex", WebDOMHTMLElement::setTabIndex)
        KSCRIPT_GUI_METHOD("getAccessKey", WebDOMHTMLElement::getAccessKey)
        KSCRIPT_GUI_METHOD("setAccessKey", WebDOMHTMLElement::setAccessKey)
    END_KSCRIPT_CLASS()
    
    
    KSCRIPT_BEGIN_PROPERTY_MAP(WebDOMHTMLElement, WebDOMElement)
        KSCRIPT_GUI_PROPERTY("value", WebDOMHTMLElement::getValue, WebDOMHTMLElement::setValue)
    KSCRIPT_END_PROPERTY_MAP()
    
public:

    WebDOMHTMLElement();
    ~WebDOMHTMLElement();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void getId(kscript::ExprEnv* env, kscript::Value* retval);
    void setId(kscript::ExprEnv* env, kscript::Value* retval);
    void getTitle(kscript::ExprEnv* env, kscript::Value* retval);
    void setTitle(kscript::ExprEnv* env, kscript::Value* retval);
    void getLang(kscript::ExprEnv* env, kscript::Value* retval);
    void setLang(kscript::ExprEnv* env, kscript::Value* retval);
    void getDir(kscript::ExprEnv* env, kscript::Value* retval);
    void setDir(kscript::ExprEnv* env, kscript::Value* retval);
    void getClassName(kscript::ExprEnv* env, kscript::Value* retval);
    void setClassName(kscript::ExprEnv* env, kscript::Value* retval);
    void getValue(kscript::ExprEnv* env, kscript::Value* retval);
    void setValue(kscript::ExprEnv* env, kscript::Value* retval);
    void getTabIndex(kscript::ExprEnv* env, kscript::Value* retval);
    void setTabIndex(kscript::ExprEnv* env, kscript::Value* retval);
    void getAccessKey(kscript::ExprEnv* env, kscript::Value* retval);
    void setAccessKey(kscript::ExprEnv* env, kscript::Value* retval);
};



class WebDOMHTMLAnchorElement: public WebDOMHTMLElement
{
    BEGIN_KSCRIPT_DERIVED_CLASS("WebDOMHTMLAnchorElement", WebDOMHTMLAnchorElement, WebDOMHTMLElement)
        KSCRIPT_GUI_METHOD("constructor", WebDOMHTMLAnchorElement::constructor)
        KSCRIPT_GUI_METHOD("getCharset", WebDOMHTMLAnchorElement::getCharset)
        KSCRIPT_GUI_METHOD("setCharset", WebDOMHTMLAnchorElement::setCharset)
        KSCRIPT_GUI_METHOD("getCoords", WebDOMHTMLAnchorElement::getCoords)
        KSCRIPT_GUI_METHOD("setCoords", WebDOMHTMLAnchorElement::setCoords)
        KSCRIPT_GUI_METHOD("getHref", WebDOMHTMLAnchorElement::getHref)
        KSCRIPT_GUI_METHOD("setHref", WebDOMHTMLAnchorElement::setHref)
        KSCRIPT_GUI_METHOD("getHreflang", WebDOMHTMLAnchorElement::getHreflang)
        KSCRIPT_GUI_METHOD("setHreflang", WebDOMHTMLAnchorElement::setHreflang)
        KSCRIPT_GUI_METHOD("getName", WebDOMHTMLAnchorElement::getName)
        KSCRIPT_GUI_METHOD("setName", WebDOMHTMLAnchorElement::setName)
        KSCRIPT_GUI_METHOD("getRel", WebDOMHTMLAnchorElement::getRel)
        KSCRIPT_GUI_METHOD("setRel", WebDOMHTMLAnchorElement::setRel)
        KSCRIPT_GUI_METHOD("getRev", WebDOMHTMLAnchorElement::getRev)
        KSCRIPT_GUI_METHOD("setRev", WebDOMHTMLAnchorElement::setRev)
        KSCRIPT_GUI_METHOD("getShape", WebDOMHTMLAnchorElement::getShape)
        KSCRIPT_GUI_METHOD("setShape", WebDOMHTMLAnchorElement::setShape)
        KSCRIPT_GUI_METHOD("getTarget", WebDOMHTMLAnchorElement::getTarget)
        KSCRIPT_GUI_METHOD("setTarget", WebDOMHTMLAnchorElement::setTarget)
        KSCRIPT_GUI_METHOD("getType", WebDOMHTMLAnchorElement::getType)
        KSCRIPT_GUI_METHOD("setType", WebDOMHTMLAnchorElement::setType)
        KSCRIPT_GUI_METHOD("blur", WebDOMHTMLAnchorElement::blur)
        KSCRIPT_GUI_METHOD("focus", WebDOMHTMLAnchorElement::focus)
    END_KSCRIPT_CLASS()

public:

    WebDOMHTMLAnchorElement();
    ~WebDOMHTMLAnchorElement();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void getCharset(kscript::ExprEnv* env, kscript::Value* retval);
    void setCharset(kscript::ExprEnv* env, kscript::Value* retval);
    void getCoords(kscript::ExprEnv* env, kscript::Value* retval);
    void setCoords(kscript::ExprEnv* env, kscript::Value* retval);
    void getHref(kscript::ExprEnv* env, kscript::Value* retval);
    void setHref(kscript::ExprEnv* env, kscript::Value* retval);
    void getHreflang(kscript::ExprEnv* env, kscript::Value* retval);
    void setHreflang(kscript::ExprEnv* env, kscript::Value* retval);
    void getName(kscript::ExprEnv* env, kscript::Value* retval);
    void setName(kscript::ExprEnv* env, kscript::Value* retval);
    void getRel(kscript::ExprEnv* env, kscript::Value* retval);
    void setRel(kscript::ExprEnv* env, kscript::Value* retval);
    void getRev(kscript::ExprEnv* env, kscript::Value* retval);
    void setRev(kscript::ExprEnv* env, kscript::Value* retval);
    void getShape(kscript::ExprEnv* env, kscript::Value* retval);
    void setShape(kscript::ExprEnv* env, kscript::Value* retval);
    void getTarget(kscript::ExprEnv* env, kscript::Value* retval);
    void setTarget(kscript::ExprEnv* env, kscript::Value* retval);
    void getType(kscript::ExprEnv* env, kscript::Value* retval);
    void setType(kscript::ExprEnv* env, kscript::Value* retval);
    void blur(kscript::ExprEnv* env, kscript::Value* retval);
    void focus(kscript::ExprEnv* env, kscript::Value* retval);
};



class WebDOMHTMLButtonElement : public WebDOMHTMLElement
{
    BEGIN_KSCRIPT_DERIVED_CLASS("WebDOMHTMLButtonElement", WebDOMHTMLButtonElement, WebDOMHTMLElement)
        KSCRIPT_GUI_METHOD("constructor", WebDOMHTMLButtonElement::constructor)
        //KSCRIPT_GUI_METHOD("getForm", WebDOMHTMLButtonElement::getForm)
        KSCRIPT_GUI_METHOD("getDisabled", WebDOMHTMLButtonElement::getDisabled)
        KSCRIPT_GUI_METHOD("setDisabled", WebDOMHTMLButtonElement::setDisabled)
        KSCRIPT_GUI_METHOD("getName", WebDOMHTMLButtonElement::getName)
        KSCRIPT_GUI_METHOD("setName", WebDOMHTMLButtonElement::setName)
        KSCRIPT_GUI_METHOD("getType", WebDOMHTMLButtonElement::getType)
    END_KSCRIPT_CLASS()
    
public:

    WebDOMHTMLButtonElement();
    ~WebDOMHTMLButtonElement();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    //void getForm(kscript::ExprEnv* env, kscript::Value* retval);
    void getDisabled(kscript::ExprEnv* env, kscript::Value* retval);
    void setDisabled(kscript::ExprEnv* env, kscript::Value* retval);
    void getName(kscript::ExprEnv* env, kscript::Value* retval);
    void setName(kscript::ExprEnv* env, kscript::Value* retval);
    void getType(kscript::ExprEnv* env, kscript::Value* retval);
};



class WebDOMHTMLInputElement : public WebDOMHTMLElement
{
    BEGIN_KSCRIPT_DERIVED_CLASS("WebDOMHTMLInputElement", WebDOMHTMLInputElement, WebDOMHTMLElement)
        KSCRIPT_GUI_METHOD("constructor", WebDOMHTMLInputElement::constructor)
        KSCRIPT_GUI_METHOD("getDefaultValue", WebDOMHTMLInputElement::getDefaultValue)
        KSCRIPT_GUI_METHOD("setDefaultValue", WebDOMHTMLInputElement::setDefaultValue)
        KSCRIPT_GUI_METHOD("getDefaultChecked", WebDOMHTMLInputElement::getDefaultChecked)
        KSCRIPT_GUI_METHOD("setDefaultChecked", WebDOMHTMLInputElement::setDefaultChecked)
        // KSCRIPT_GUI_METHOD("getForm", WebDOMHTMLInputElement::getForm)
        KSCRIPT_GUI_METHOD("getAccept", WebDOMHTMLInputElement::getAccept)
        KSCRIPT_GUI_METHOD("setAccept", WebDOMHTMLInputElement::setAccept)
        KSCRIPT_GUI_METHOD("getAlign", WebDOMHTMLInputElement::getAlign)
        KSCRIPT_GUI_METHOD("setAlign", WebDOMHTMLInputElement::setAlign)
        KSCRIPT_GUI_METHOD("getAlt", WebDOMHTMLInputElement::getAlt)
        KSCRIPT_GUI_METHOD("setAlt", WebDOMHTMLInputElement::setAlt)
        KSCRIPT_GUI_METHOD("getChecked", WebDOMHTMLInputElement::getChecked)
        KSCRIPT_GUI_METHOD("setChecked", WebDOMHTMLInputElement::setChecked)
        KSCRIPT_GUI_METHOD("getDisabled", WebDOMHTMLInputElement::getDisabled)
        KSCRIPT_GUI_METHOD("setDisabled", WebDOMHTMLInputElement::setDisabled)
        KSCRIPT_GUI_METHOD("getMaxLength", WebDOMHTMLInputElement::getMaxLength)
        KSCRIPT_GUI_METHOD("setMaxLength", WebDOMHTMLInputElement::setMaxLength)
        KSCRIPT_GUI_METHOD("getName", WebDOMHTMLInputElement::getName)
        KSCRIPT_GUI_METHOD("setName", WebDOMHTMLInputElement::setName)
        KSCRIPT_GUI_METHOD("getReadOnly", WebDOMHTMLInputElement::getReadOnly)
        KSCRIPT_GUI_METHOD("setReadOnly", WebDOMHTMLInputElement::setReadOnly)
        KSCRIPT_GUI_METHOD("getSize", WebDOMHTMLInputElement::getSize)
        KSCRIPT_GUI_METHOD("setSize", WebDOMHTMLInputElement::setSize)
        KSCRIPT_GUI_METHOD("getSrc", WebDOMHTMLInputElement::getSrc)
        KSCRIPT_GUI_METHOD("setSrc", WebDOMHTMLInputElement::setSrc)
        KSCRIPT_GUI_METHOD("getType", WebDOMHTMLInputElement::getType)
        KSCRIPT_GUI_METHOD("setType", WebDOMHTMLInputElement::setType)
        KSCRIPT_GUI_METHOD("getUseMap", WebDOMHTMLInputElement::getUseMap)
        KSCRIPT_GUI_METHOD("setUseMap", WebDOMHTMLInputElement::setUseMap)
        KSCRIPT_GUI_METHOD("blur", WebDOMHTMLInputElement::blur)
        KSCRIPT_GUI_METHOD("focus", WebDOMHTMLInputElement::focus)
        KSCRIPT_GUI_METHOD("select", WebDOMHTMLInputElement::select)
        KSCRIPT_GUI_METHOD("click", WebDOMHTMLInputElement::click)
    END_KSCRIPT_CLASS()
    
public:

    WebDOMHTMLInputElement();
    ~WebDOMHTMLInputElement();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void getDefaultValue(kscript::ExprEnv* env, kscript::Value* retval);
    void setDefaultValue(kscript::ExprEnv* env, kscript::Value* retval);
    void getDefaultChecked(kscript::ExprEnv* env, kscript::Value* retval);
    void setDefaultChecked(kscript::ExprEnv* env, kscript::Value* retval);
    // void getForm(kscript::ExprEnv* env, kscript::Value* retval);
    void getAccept(kscript::ExprEnv* env, kscript::Value* retval);
    void setAccept(kscript::ExprEnv* env, kscript::Value* retval);
    void getAlign(kscript::ExprEnv* env, kscript::Value* retval);
    void setAlign(kscript::ExprEnv* env, kscript::Value* retval);
    void getAlt(kscript::ExprEnv* env, kscript::Value* retval);
    void setAlt(kscript::ExprEnv* env, kscript::Value* retval);
    void getChecked(kscript::ExprEnv* env, kscript::Value* retval);
    void setChecked(kscript::ExprEnv* env, kscript::Value* retval);
    void getDisabled(kscript::ExprEnv* env, kscript::Value* retval);
    void setDisabled(kscript::ExprEnv* env, kscript::Value* retval);
    void getMaxLength(kscript::ExprEnv* env, kscript::Value* retval);
    void setMaxLength(kscript::ExprEnv* env, kscript::Value* retval);
    void getName(kscript::ExprEnv* env, kscript::Value* retval);
    void setName(kscript::ExprEnv* env, kscript::Value* retval);
    void getReadOnly(kscript::ExprEnv* env, kscript::Value* retval);
    void setReadOnly(kscript::ExprEnv* env, kscript::Value* retval);
    void getSize(kscript::ExprEnv* env, kscript::Value* retval);
    void setSize(kscript::ExprEnv* env, kscript::Value* retval);
    void getSrc(kscript::ExprEnv* env, kscript::Value* retval);
    void setSrc(kscript::ExprEnv* env, kscript::Value* retval);
    void getType(kscript::ExprEnv* env, kscript::Value* retval);
    void setType(kscript::ExprEnv* env, kscript::Value* retval);
    void getUseMap(kscript::ExprEnv* env, kscript::Value* retval);
    void setUseMap(kscript::ExprEnv* env, kscript::Value* retval);
    void blur(kscript::ExprEnv* env, kscript::Value* retval);
    void focus(kscript::ExprEnv* env, kscript::Value* retval);
    void select(kscript::ExprEnv* env, kscript::Value* retval);
    void click(kscript::ExprEnv* env, kscript::Value* retval);
};



class WebDOMHTMLLinkElement : public WebDOMHTMLElement
{
    BEGIN_KSCRIPT_DERIVED_CLASS("WebDOMHTMLLinkElement", WebDOMHTMLLinkElement, WebDOMHTMLElement)
        KSCRIPT_GUI_METHOD("constructor", WebDOMHTMLLinkElement::constructor)
        KSCRIPT_GUI_METHOD("getDisabled", WebDOMHTMLLinkElement::getDisabled)
        KSCRIPT_GUI_METHOD("setDisabled", WebDOMHTMLLinkElement::setDisabled)
        KSCRIPT_GUI_METHOD("getCharset", WebDOMHTMLLinkElement::getCharset)
        KSCRIPT_GUI_METHOD("setCharset", WebDOMHTMLLinkElement::setCharset)
        KSCRIPT_GUI_METHOD("getHref", WebDOMHTMLLinkElement::getHref)
        KSCRIPT_GUI_METHOD("setHref", WebDOMHTMLLinkElement::setHref)
        KSCRIPT_GUI_METHOD("getHreflang", WebDOMHTMLLinkElement::getHreflang)
        KSCRIPT_GUI_METHOD("setHreflang", WebDOMHTMLLinkElement::setHreflang)
        KSCRIPT_GUI_METHOD("getMedia", WebDOMHTMLLinkElement::getMedia)
        KSCRIPT_GUI_METHOD("setMedia", WebDOMHTMLLinkElement::setMedia)
        KSCRIPT_GUI_METHOD("getRel", WebDOMHTMLLinkElement::getRel)
        KSCRIPT_GUI_METHOD("setRel", WebDOMHTMLLinkElement::setRel)
        KSCRIPT_GUI_METHOD("getRev", WebDOMHTMLLinkElement::getRev)
        KSCRIPT_GUI_METHOD("setRev", WebDOMHTMLLinkElement::setRev)
        KSCRIPT_GUI_METHOD("getTarget", WebDOMHTMLLinkElement::getTarget)
        KSCRIPT_GUI_METHOD("setTarget", WebDOMHTMLLinkElement::setTarget)
        KSCRIPT_GUI_METHOD("getType", WebDOMHTMLLinkElement::getType)
        KSCRIPT_GUI_METHOD("setType", WebDOMHTMLLinkElement::setType)
    END_KSCRIPT_CLASS()
    
public:

    WebDOMHTMLLinkElement();
    ~WebDOMHTMLLinkElement();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void getDisabled(kscript::ExprEnv* env, kscript::Value* retval);
    void setDisabled(kscript::ExprEnv* env, kscript::Value* retval);
    void getCharset(kscript::ExprEnv* env, kscript::Value* retval);
    void setCharset(kscript::ExprEnv* env, kscript::Value* retval);
    void getHref(kscript::ExprEnv* env, kscript::Value* retval);
    void setHref(kscript::ExprEnv* env, kscript::Value* retval);
    void getHreflang(kscript::ExprEnv* env, kscript::Value* retval);
    void setHreflang(kscript::ExprEnv* env, kscript::Value* retval);
    void getMedia(kscript::ExprEnv* env, kscript::Value* retval);
    void setMedia(kscript::ExprEnv* env, kscript::Value* retval);
    void getRel(kscript::ExprEnv* env, kscript::Value* retval);
    void setRel(kscript::ExprEnv* env, kscript::Value* retval);
    void getRev(kscript::ExprEnv* env, kscript::Value* retval);
    void setRev(kscript::ExprEnv* env, kscript::Value* retval);
    void getTarget(kscript::ExprEnv* env, kscript::Value* retval);
    void setTarget(kscript::ExprEnv* env, kscript::Value* retval);
    void getType(kscript::ExprEnv* env, kscript::Value* retval);
    void setType(kscript::ExprEnv* env, kscript::Value* retval);
};



class WebDOMHTMLOptionElement : public WebDOMHTMLElement
{
    BEGIN_KSCRIPT_DERIVED_CLASS("WebDOMHTMLOptionElement", WebDOMHTMLOptionElement, WebDOMHTMLElement)
        KSCRIPT_GUI_METHOD("constructor", WebDOMHTMLOptionElement::constructor)
        KSCRIPT_GUI_METHOD("getDefaultSelected", WebDOMHTMLOptionElement::getDefaultSelected)
        KSCRIPT_GUI_METHOD("setDefaultSelected", WebDOMHTMLOptionElement::setDefaultSelected)
        KSCRIPT_GUI_METHOD("getText", WebDOMHTMLOptionElement::getText)
        KSCRIPT_GUI_METHOD("getIndex", WebDOMHTMLOptionElement::getIndex)
        KSCRIPT_GUI_METHOD("getDisabled", WebDOMHTMLOptionElement::getDisabled)
        KSCRIPT_GUI_METHOD("setDisabled", WebDOMHTMLOptionElement::setDisabled)
        KSCRIPT_GUI_METHOD("getLabel", WebDOMHTMLOptionElement::getLabel)
        KSCRIPT_GUI_METHOD("setLabel", WebDOMHTMLOptionElement::setLabel)
        KSCRIPT_GUI_METHOD("getSelected", WebDOMHTMLOptionElement::getSelected)
        KSCRIPT_GUI_METHOD("setSelected", WebDOMHTMLOptionElement::setSelected)
    END_KSCRIPT_CLASS()
    
public:

    WebDOMHTMLOptionElement();
    ~WebDOMHTMLOptionElement();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void getDefaultSelected(kscript::ExprEnv* env, kscript::Value* retval);
    void setDefaultSelected(kscript::ExprEnv* env, kscript::Value* retval);
    void getText(kscript::ExprEnv* env, kscript::Value* retval);
    void getIndex(kscript::ExprEnv* env, kscript::Value* retval);
    void getDisabled(kscript::ExprEnv* env, kscript::Value* retval);
    void setDisabled(kscript::ExprEnv* env, kscript::Value* retval);
    void getLabel(kscript::ExprEnv* env, kscript::Value* retval);
    void setLabel(kscript::ExprEnv* env, kscript::Value* retval);
    void getSelected(kscript::ExprEnv* env, kscript::Value* retval);
    void setSelected(kscript::ExprEnv* env, kscript::Value* retval);
};



class WebDOMHTMLSelectElement : public WebDOMHTMLElement
{
    BEGIN_KSCRIPT_DERIVED_CLASS("WebDOMHTMLSelectElement", WebDOMHTMLSelectElement, WebDOMHTMLElement)
        KSCRIPT_GUI_METHOD("constructor", WebDOMHTMLSelectElement::constructor)
        KSCRIPT_GUI_METHOD("getType", WebDOMHTMLSelectElement::getType)
        KSCRIPT_GUI_METHOD("getSelectedIndex", WebDOMHTMLSelectElement::getSelectedIndex)
        KSCRIPT_GUI_METHOD("setSelectedIndex", WebDOMHTMLSelectElement::setSelectedIndex)
        KSCRIPT_GUI_METHOD("getLength", WebDOMHTMLSelectElement::getLength)
        KSCRIPT_GUI_METHOD("setLength", WebDOMHTMLSelectElement::setLength)
        // KSCRIPT_GUI_METHOD("getForm", WebDOMHTMLSelectElement::getForm)
        // KSCRIPT_GUI_METHOD("getOptions", WebDOMHTMLSelectElement::getOptions)
        KSCRIPT_GUI_METHOD("getDisabled", WebDOMHTMLSelectElement::getDisabled)
        KSCRIPT_GUI_METHOD("setDisabled", WebDOMHTMLSelectElement::setDisabled)
        KSCRIPT_GUI_METHOD("getMultiple", WebDOMHTMLSelectElement::getMultiple)
        KSCRIPT_GUI_METHOD("setMultiple", WebDOMHTMLSelectElement::setMultiple)
        KSCRIPT_GUI_METHOD("getName", WebDOMHTMLSelectElement::getName)
        KSCRIPT_GUI_METHOD("setName", WebDOMHTMLSelectElement::setName)
        KSCRIPT_GUI_METHOD("getSize", WebDOMHTMLSelectElement::getSize)
        KSCRIPT_GUI_METHOD("setSize", WebDOMHTMLSelectElement::setSize)
        KSCRIPT_GUI_METHOD("add", WebDOMHTMLSelectElement::add)
        KSCRIPT_GUI_METHOD("remove", WebDOMHTMLSelectElement::remove)
        KSCRIPT_GUI_METHOD("blur", WebDOMHTMLSelectElement::blur)
        KSCRIPT_GUI_METHOD("focus", WebDOMHTMLSelectElement::focus)
    END_KSCRIPT_CLASS()
    
public:

    WebDOMHTMLSelectElement();
    ~WebDOMHTMLSelectElement();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void getType(kscript::ExprEnv* env, kscript::Value* retval);
    void getSelectedIndex(kscript::ExprEnv* env, kscript::Value* retval);
    void setSelectedIndex(kscript::ExprEnv* env, kscript::Value* retval);
    void getLength(kscript::ExprEnv* env, kscript::Value* retval);
    void setLength(kscript::ExprEnv* env, kscript::Value* retval);
    // void getForm(kscript::ExprEnv* env, kscript::Value* retval);
    // void getOptions(kscript::ExprEnv* env, kscript::Value* retval);
    void getDisabled(kscript::ExprEnv* env, kscript::Value* retval);
    void setDisabled(kscript::ExprEnv* env, kscript::Value* retval);
    void getMultiple(kscript::ExprEnv* env, kscript::Value* retval);
    void setMultiple(kscript::ExprEnv* env, kscript::Value* retval);
    void getName(kscript::ExprEnv* env, kscript::Value* retval);
    void setName(kscript::ExprEnv* env, kscript::Value* retval);
    void getSize(kscript::ExprEnv* env, kscript::Value* retval);
    void setSize(kscript::ExprEnv* env, kscript::Value* retval);
    void add(kscript::ExprEnv* env, kscript::Value* retval);
    void remove(kscript::ExprEnv* env, kscript::Value* retval);
    void blur(kscript::ExprEnv* env, kscript::Value* retval);
    void focus(kscript::ExprEnv* env, kscript::Value* retval);
};



class WebDOMHTMLTextAreaElement : public WebDOMHTMLElement
{
    BEGIN_KSCRIPT_DERIVED_CLASS("WebDOMHTMLTextAreaElement", WebDOMHTMLTextAreaElement, WebDOMHTMLElement)
        KSCRIPT_GUI_METHOD("constructor", WebDOMHTMLTextAreaElement::constructor)
        KSCRIPT_GUI_METHOD("getDefaultValue", WebDOMHTMLTextAreaElement::getDefaultValue)
        KSCRIPT_GUI_METHOD("setDefaultValue", WebDOMHTMLTextAreaElement::setDefaultValue)
        // KSCRIPT_GUI_METHOD("getForm", WebDOMHTMLTextAreaElement::getForm)
        KSCRIPT_GUI_METHOD("getCols", WebDOMHTMLTextAreaElement::getCols)
        KSCRIPT_GUI_METHOD("setCols", WebDOMHTMLTextAreaElement::setCols)
        KSCRIPT_GUI_METHOD("getDisabled", WebDOMHTMLTextAreaElement::getDisabled)
        KSCRIPT_GUI_METHOD("setDisabled", WebDOMHTMLTextAreaElement::setDisabled)
        KSCRIPT_GUI_METHOD("getName", WebDOMHTMLTextAreaElement::getName)
        KSCRIPT_GUI_METHOD("setName", WebDOMHTMLTextAreaElement::setName)
        KSCRIPT_GUI_METHOD("getReadOnly", WebDOMHTMLTextAreaElement::getReadOnly)
        KSCRIPT_GUI_METHOD("setReadOnly", WebDOMHTMLTextAreaElement::setReadOnly)
        KSCRIPT_GUI_METHOD("getRows", WebDOMHTMLTextAreaElement::getRows)
        KSCRIPT_GUI_METHOD("setRows", WebDOMHTMLTextAreaElement::setRows)
        KSCRIPT_GUI_METHOD("getType", WebDOMHTMLTextAreaElement::getType)       
        KSCRIPT_GUI_METHOD("blur", WebDOMHTMLTextAreaElement::blur)
        KSCRIPT_GUI_METHOD("focus", WebDOMHTMLTextAreaElement::focus)
        KSCRIPT_GUI_METHOD("select", WebDOMHTMLTextAreaElement::select)
    END_KSCRIPT_CLASS()
    
public:

    WebDOMHTMLTextAreaElement();
    ~WebDOMHTMLTextAreaElement();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void getDefaultValue(kscript::ExprEnv* env, kscript::Value* retval);
    void setDefaultValue(kscript::ExprEnv* env, kscript::Value* retval);
    // void getForm(kscript::ExprEnv* env, kscript::Value* retval);
    void getCols(kscript::ExprEnv* env, kscript::Value* retval);
    void setCols(kscript::ExprEnv* env, kscript::Value* retval);
    void getDisabled(kscript::ExprEnv* env, kscript::Value* retval);
    void setDisabled(kscript::ExprEnv* env, kscript::Value* retval);
    void getName(kscript::ExprEnv* env, kscript::Value* retval);
    void setName(kscript::ExprEnv* env, kscript::Value* retval);
    void getReadOnly(kscript::ExprEnv* env, kscript::Value* retval);
    void setReadOnly(kscript::ExprEnv* env, kscript::Value* retval);
    void getRows(kscript::ExprEnv* env, kscript::Value* retval);
    void setRows(kscript::ExprEnv* env, kscript::Value* retval);
    void getType(kscript::ExprEnv* env, kscript::Value* retval);
    void blur(kscript::ExprEnv* env, kscript::Value* retval);
    void focus(kscript::ExprEnv* env, kscript::Value* retval);
    void select(kscript::ExprEnv* env, kscript::Value* retval);
};



#endif

