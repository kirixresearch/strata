/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2008-02-28
 *
 */


#include "appmain.h"
#include "scriptgui.h"
#include "scriptwebdom.h"


static WebDOMNode* createAppropriateDOMNode(kscript::ExprEnv* env, wxDOMNode& node)
{
    if (!node.IsOk())
        return (WebDOMNode::createObject(env));
        
    wxString name = node.GetNodeName();
    name.MakeUpper();
    
    // TODO: verify these node names (i.e., ANCHOR, etc)
    if (name == wxT("ANCHOR"))
        return (WebDOMHTMLAnchorElement::createObject(env));
    if (name == wxT("BUTTON"))
        return (WebDOMHTMLButtonElement::createObject(env));
    if (name == wxT("INPUT"))
        return (WebDOMHTMLInputElement::createObject(env));
    if (name == wxT("LINK"))
        return (WebDOMHTMLLinkElement::createObject(env));
    if (name == wxT("OPTION"))
        return (WebDOMHTMLOptionElement::createObject(env));
    if (name == wxT("SELECT"))
        return (WebDOMHTMLSelectElement::createObject(env));
    if (name == wxT("TEXTAREA"))
        return (WebDOMHTMLTextAreaElement::createObject(env));
        
    switch (node.GetNodeType())
    {
        case wxWEB_NODETYPE_ELEMENT_NODE: return (WebDOMElement::createObject(env));
        case wxWEB_NODETYPE_ATTRIBUTE_NODE: return (WebDOMAttr::createObject(env));
        case wxWEB_NODETYPE_TEXT_NODE: return (WebDOMText::createObject(env));
        case wxWEB_NODETYPE_DOCUMENT_NODE: return (WebDOMDocument::createObject(env));
        default:    return (WebDOMNode::createObject(env));

    }
    
    return (WebDOMNode::createObject(env));
}


// (CLASS) WebDOMNode
// Category: DOM
// Derives:
// Description: A class that represents a DOM node.
// Remarks: The WebDOMNode class represents a DOM node.
//
// Property(Node.ELEMENT_NODE):                 Constant representing an element node in the DOM.
// Property(Node.ATTRIBUTE_NODE):               Constant representing an attribute node in the DOM.
// Property(Node.TEXT_NODE):                    Constant representing a text node in the DOM.
// Property(Node.CDATA_SECTION_NODE):           Constant representing a CDATA section node in the DOM.
// Property(Node.ENTITY_REFERENCE_NODE):        Constant representing an entity reference node in the DOM.
// Property(Node.ENTITY_NODE):                  Constant representing an entity node in the DOM.
// Property(Node.PROCESSING_INSTRUCTION_NODE):  Constant representing a processing instruction node in the DOM.
// Property(Node.COMMENT_NODE):                 Constant representing a comment node in the DOM.
// Property(Node.DOCUMENT_NODE):                Constant representing a document node in the DOM.
// Property(Node.DOCUMENT_TYPE_NODE):           Constant representing a document type node in the DOM.
// Property(Node.DOCUMENT_FRAGMENT_NODE):       Constant representing a document fragment node in the DOM.
// Property(Node.NOTATION_NODE):                Constant representing a notation node in the DOM.

WebDOMNode::WebDOMNode()
{
}

WebDOMNode::~WebDOMNode()
{
}

void WebDOMNode::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

// (METHOD) WebDOMNode.getOwnerDocument
// Description: Gets the document to which the node belongs.
//
// Syntax: WebDOMNode.getOwnerDocument() : wxDOMDocument
//
// Remarks: Returns a WebDOMDocument object that represents 
//     the document to which the node belongs.
//
// Returns: A WebDOMDocument object that represents the document
//     to which the node belongs.

void WebDOMNode::getOwnerDocument(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();

    if (!m_node.IsOk())
        return;

    wxDOMDocument result;
    result = m_node.GetOwnerDocument();
    if (result.IsOk())
    {
        WebDOMDocument* res = WebDOMDocument::createObject(env);
        res->m_node = result;
        retval->setObject(res);
    }
}

// (METHOD) WebDOMNode.getNodeName
// Description: Gets the name of this node.
//
// Syntax: WebDOMNode.getNodeName() : String
//
// Remarks: Returns the name of this node.
//
// Returns: Returns the name of this node.

void WebDOMNode::getNodeName(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();

    if (m_node.IsOk())
    {
        retval->setString(m_node.GetNodeName().wc_str());
    }
}

// (METHOD) WebDOMNode.getNodeType
// Description: Gets the type of this node.
//
// Syntax: WebDOMNode.getNodeType() : Integer
//
// Remarks: Returns the type of this node.
//
// Returns: Returns the type of this node.

void WebDOMNode::getNodeType(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();
    
    if (m_node.IsOk())
    {
        retval->setInteger(m_node.GetNodeType());
    }
}

// (METHOD) WebDOMNode.getNodeValue
// Description: Gets the value of this node.
//
// Syntax: WebDOMNode.getNodeValue() : String
//
// Remarks: Returns the value of this node.
//
// Returns: Returns the value of this node.

void WebDOMNode::getNodeValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();

    if (m_node.IsOk())
    {
        retval->setString(m_node.GetNodeValue().wc_str());
    }
}

// (METHOD) WebDOMNode.setNodeValue
// Description: Sets the value of this node.
//
// Syntax: WebDOMNode.getNodeValue(text : String)
//
// Remarks: Sets the value of this node to the specified |text|.
//
// Param(value): The |text| to which to set the value of this node.

void WebDOMNode::setNodeValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_node.IsOk() && env->getParamCount() > 0)
    {
        wxString value = env->getParam(0)->getString();
        m_node.SetNodeValue(value);
    }
}

// (METHOD) WebDOMNode.getParentNode
// Description: Gets the parent node of this node.
//
// Syntax: WebDOMNode.getParentNode() : WebDOMNode
//
// Remarks: Returns the parent node of this node as a
//     WebDOMNode object.
//
// Returns: The parent node of this node as a WebDOMNode
//     object.

void WebDOMNode::getParentNode(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();

    if (!m_node.IsOk())
        return;

    wxDOMNode result;
    result = m_node.GetParentNode();
    if (result.IsOk())
    {
        WebDOMNode* res = createAppropriateDOMNode(env, result);
        res->m_node = result;
        retval->setObject(res);
    }
}

// (METHOD) WebDOMNode.getChildNodes
// Description: Gets the child nodes of this node.
//
// Syntax: WebDOMNode.getChildNodes() : Array(WebDOMNode)
//
// Remarks: Returns the children of this node as an array 
//     of WebDOMNode objects.
//
// Returns: The children of this node as an array of 
//     WebDOMNode objects.

void WebDOMNode::getChildNodes(kscript::ExprEnv* env, kscript::Value* retval)
{
    // set the default return value
    retval->setArray(env);

    if (!m_node.IsOk())
        return;
    
    wxDOMNodeList child_nodes = m_node.GetChildNodes();
    if (!child_nodes.IsOk())
        return;
    
    size_t count = child_nodes.GetLength();
    for (size_t i = 0; i < count; ++i)
    {
        wxDOMNode node = child_nodes.Item(i);
        
        WebDOMNode* child_node = createAppropriateDOMNode(env, node);
        child_node->m_node = node;
        
        kscript::Value val;
        val.setObject(child_node);
        retval->appendMember(&val);
    }
}

// (METHOD) WebDOMNode.getFirstChild
// Description: Gets the first child node of this node.
//
// Syntax: WebDOMNode.getFirstChild() : WebDOMNode
//
// Remarks: Returns the first child node of this node as a
//     WebDOMNode object.
//
// Returns: The first child node of this node as a WebDOMNode
//     object.

void WebDOMNode::getFirstChild(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();

    if (!m_node.IsOk())
        return;

    wxDOMNode result;
    result = m_node.GetFirstChild();
    if (result.IsOk())
    {
        WebDOMNode* res = createAppropriateDOMNode(env, result);
        res->m_node = result;
        retval->setObject(res);
    }
}

// (METHOD) WebDOMNode.getLastChild
// Description: Gets the last child node of this node.
//
// Syntax: WebDOMNode.getLastChild() : WebDOMNode
//
// Remarks: Returns the last child node of this node as a
//     WebDOMNode object.
//
// Returns: The last child node of this node as a WebDOMNode
//     object.

void WebDOMNode::getLastChild(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();

    if (!m_node.IsOk())
        return;

    wxDOMNode result;
    result = m_node.GetLastChild();
    if (result.IsOk())
    {
        WebDOMNode* res = createAppropriateDOMNode(env, result);
        res->m_node = result;
        retval->setObject(res);
    }
}

// (METHOD) WebDOMNode.getPreviousSibling
// Description: Gets the previous sibling node of this node.
//
// Syntax: WebDOMNode.getPreviousSibling() : WebDOMNode
//
// Remarks: Returns the previous sibling node of this node as a
//     WebDOMNode object.
//
// Returns: The previous sibling node of this node as a WebDOMNode
//     object.

void WebDOMNode::getPreviousSibling(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();

    if (!m_node.IsOk())
        return;

    wxDOMNode result;
    result = m_node.GetPreviousSibling();
    if (result.IsOk())
    {
        WebDOMNode* res = createAppropriateDOMNode(env, result);
        res->m_node = result;
        retval->setObject(res);
    }
}

// (METHOD) WebDOMNode.getNextSibling
// Description: Gets the next sibling node of this node.
//
// Syntax: WebDOMNode.getNextSibling() : WebDOMNode
//
// Remarks: Returns the next sibling node of this node as a
//     WebDOMNode object.
//
// Returns: The next sibling node of this node as a WebDOMNode
//     object.

void WebDOMNode::getNextSibling(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();

    if (!m_node.IsOk())
        return;

    wxDOMNode result;
    result = m_node.GetNextSibling();
    if (result.IsOk())
    {
        WebDOMNode* res = createAppropriateDOMNode(env, result);
        res->m_node = result;
        retval->setObject(res);
    }
}

// (METHOD) WebDOMNode.insertBefore
// Description: Inserts a child node before another child node
//     within this node's children.
//
// Syntax: WebDOMNode.insertBefore(new_child : WebDOMNode,
//                                 ref_child : WebDOMNode) : WebDOMNode
//
// Remarks: Inserts a |new_child| node before another |ref_child| node
//     within this node's children, and returns the newly inserted
//     node as a WebDOMNode object.  If |ref_child| is null, the 
//     |new_child| is inserted as the last object of this node.
//
// Param(new_child): The node to insert.
// Param(ref_child): The node before which to insert the
//     |new_child| node.
//
// Returns: The newly inserted node.

void WebDOMNode::insertBefore(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();

    if (!m_node.IsOk())
        return;
    
    if (env->getParamCount() < 1)
        return;
        
    if (!env->getParam(0)->isObject())
        return;
        
    kscript::ValueObject* vobj1 = env->getParam(0)->getObject();
    if (!vobj1->isKindOf(L"WebDOMNode"))
        return;

    // if we don't have a second parameter, or it isn't an
    // object, or it isn't a WebDOMNode object, simply append
    // the child to the node
    if (env->getParamCount() < 2 ||
        !env->getParam(1)->isObject() ||
        !env->getParam(1)->getObject()->isKindOf(L"WebDOMNode"))
    {
        WebDOMNode* new_child = (WebDOMNode*)vobj1;
    
        wxDOMNode result;
        result = m_node.AppendChild(new_child->m_node);
        if (result.IsOk())
        {
            WebDOMNode* res = createAppropriateDOMNode(env, result);
            res->m_node = result;
            retval->setObject(res);
        }
        
        return;
    }
    
    // we have reference node as a second parameter, so insert the 
    // new child before this node
    kscript::ValueObject* vobj2 = env->getParam(1)->getObject();
    
    WebDOMNode* new_child = (WebDOMNode*)vobj1;
    WebDOMNode* ref_child = (WebDOMNode*)vobj2;
    
    wxDOMNode result;
    result = m_node.InsertBefore(new_child->m_node, ref_child->m_node);
    if (result.IsOk())
    {
        WebDOMNode* res = createAppropriateDOMNode(env, result);
        res->m_node = result;
        retval->setObject(res);
    }
}

// (METHOD) WebDOMNode.replaceChild
// Description: Replaces a child node of this node with another node.
//
// Syntax: WebDOMNode.replaceChild(new_child : WebDOMNode,
//                                 old_child : WebDOMNode) : WebDOMNode
//
// Remarks: Replaces the |old_child| node with the |new_child| node, and
//     returns the |old_child| node which was replaced.
//
// Param(new_child): The new node with which to replace the old node.
// Param(old_child): The old node which is to be replaced.
//
// Returns: The node which was replaced.

void WebDOMNode::replaceChild(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();

    if (!m_node.IsOk())
        return;
    
    // if we don't have 2 parameters, we're done
    if (env->getParamCount() < 2)
        return;

    // if either of the parameters aren't objects, we're done        
    if (!env->getParam(0)->isObject() || 
        !env->getParam(1)->isObject())
    {
        return;
    }

    // if either of the parameters aren't WebDOMNodes, we're done        
    kscript::ValueObject* vobj1 = env->getParam(0)->getObject();
    kscript::ValueObject* vobj2 = env->getParam(1)->getObject();
        
    if (!vobj1->isKindOf(L"WebDOMNode") ||
        !vobj2->isKindOf(L"WebDOMNode"))
    {
        return;
    }

    // replace the old child with the new child
    WebDOMNode* new_child = (WebDOMNode*)vobj1;
    WebDOMNode* old_child = (WebDOMNode*)vobj2;
    
    wxDOMNode result;
    result = m_node.ReplaceChild(new_child->m_node, old_child->m_node);
    if (result.IsOk())
    {
        WebDOMNode* res = createAppropriateDOMNode(env, result);
        res->m_node = result;
        retval->setObject(res);
    }
}

// (METHOD) WebDOMNode.removeChild
// Description: Removes a child node from this node.
//
// Syntax: WebDOMNode.removeChild(old_child : WebDOMNode) : WebDOMNode
//
// Remarks: Removes the |old_child| node from this node, and returns
//     the newly removed node.
//
// Param(old_child): The child node to remove.
//
// Returns: The child node which was removed.

void WebDOMNode::removeChild(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();

    if (!m_node.IsOk())
        return;
    
    if (env->getParamCount() < 1)
        return;
        
    if (!env->getParam(0)->isObject())
        return;

    kscript::ValueObject* vobj = env->getParam(0)->getObject();
    if (!vobj->isKindOf(L"WebDOMNode"))
        return;

    WebDOMNode* child_node = (WebDOMNode*)vobj;

    wxDOMNode result;
    result = m_node.RemoveChild(child_node->m_node);
    if (result.IsOk())
    {
        WebDOMNode* res = createAppropriateDOMNode(env, result);
        res->m_node = result;
        retval->setObject(res);
    }
}

// (METHOD) WebDOMNode.appendChild
// Description: Appends a child node to this node.
//
// Syntax: WebDOMNode.appendChild(new_child : WebDOMNode) : WebDOMNode
//
// Remarks: Appends the |new_child| node to this node, and returns this
//     newly appended child node.  If the appended node already exists
//     in the list of child nodes, it is moved to the end of that list.
//
// Remarks: Appends a node to the list of children nodes.  

// Param(new_child): The child node to append.
//
// Returns: The child node that was appended.

void WebDOMNode::appendChild(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();

    if (!m_node.IsOk())
        return;
    
    if (env->getParamCount() < 1 || !env->getParam(0)->isObject())
        return;
        
    kscript::ValueObject* vobj = env->getParam(0)->getObject();
    if (!vobj->isKindOf(L"WebDOMNode"))
        return;

    WebDOMNode* child_node = (WebDOMNode*)vobj;

    wxDOMNode result;
    result = m_node.AppendChild(child_node->m_node);
    if (result.IsOk())
    {
        WebDOMNode* res = createAppropriateDOMNode(env, result);
        res->m_node = result;
        retval->setObject(res);
    }
}

// (METHOD) WebDOMNode.cloneNode
// Description: Clones this node.
//
// Syntax: WebDOMNode.cloneNode(deep : Boolean) : WebDOMNode
//
// Remarks: Clones this node and returns it.  If |deep| is
//     true, all of this node's children are also cloned; if
//     |deep| is false, only this node is cloned, and the
//     child nodes are not copied.
//
// Param(deep): If |deep| is true, all of this node's children
//     are also cloned; if |deep| is false, only this node is
//     cloned and the child nodes are not copied.
//
// Returns: The newly cloned node.

void WebDOMNode::cloneNode(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();

    if (!m_node.IsOk())
        return;

    bool deep = false;
    if (env->getParamCount() > 0)
        deep = env->getParam(0)->getBoolean();

    wxDOMNode result;
    result = m_node.CloneNode(deep);
    if (result.IsOk())
    {
        WebDOMNode* res = createAppropriateDOMNode(env, result);
        res->m_node = result;
        retval->setObject(res);
    }
}

// (METHOD) WebDOMNode.normalize
// Description: Removes all empty text nodes and consolidates adjacent
//     text nodes into a single node.
//
// Syntax: WebDOMNode.normalize()
//
// Remarks: Removes all empty text child nodes from this node and
//     consolidates adjacent text child nodes into a single node.

void WebDOMNode::normalize(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_node.IsOk())
    {
        m_node.Normalize();
    }
}


// (METHOD) WebDOMNode.hasChildNodes
// Description: Returns true if this node has child nodes, and false otherwise.
//
// Syntax: WebDOMNode.hasChildNodes() : Boolean
//
// Remarks: Returns true if this node has child nodes, and false otherwise.
//
// Returns: Returns true if this node has child nodes, and false otherwise.

void WebDOMNode::hasChildNodes(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_node.IsOk())
    {
        retval->setBoolean(false);
        return;
    }

    retval->setBoolean(m_node.HasChildNodes());
}

// (METHOD) WebDOMNode.hasAttributes
// Description: Returns true if this node has attributes, and false otherwise.
//
// Syntax: WebDOMNode.hasAttributes() : Boolean
//
// Remarks: Returns true if this node has attributes, and false otherwise.
//
// Returns: Returns true if this node has attributes, and false otherwise.

void WebDOMNode::hasAttributes(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_node.IsOk())
    {
        retval->setBoolean(false);
        return;
    }

    retval->setBoolean(m_node.HasAttributes());
}

// (METHOD) WebDOMNode.getPrefix
// Description: Returns the prefix
//
// Syntax: WebDOMNode.getPrefix() : String
//
// Remarks: Returns the prefix.
//
// Returns: The prefix.

void WebDOMNode::getPrefix(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: review documentation

    retval->setNull();
    
    if (m_node.IsOk())
    {
        retval->setString(m_node.GetPrefix().wc_str());
    }
}

// (METHOD) WebDOMNode.setPrefix
// Description: Sets the prefix
//
// Syntax: WebDOMNode.setPrefix(value : String)
//
// Remarks: Sets the prefix.
//
// Param(value): The |value| to which to set the prefix.

void WebDOMNode::setPrefix(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: review documentation

    if (m_node.IsOk() && env->getParamCount() > 0)
    {
        wxString value = env->getParam(0)->getString();
        m_node.SetPrefix(value);
    }
}

// (METHOD) WebDOMNode.getNamespaceURI
// Description: Returns the namespace URI
//
// Syntax: WebDOMNode.getNamespaceURI() : String
//
// Remarks: Returns the namespace URI.
//
// Returns: The namespace URI.

void WebDOMNode::getNamespaceURI(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: review documentation

    retval->setNull();
    
    if (m_node.IsOk())
    {
        retval->setString(m_node.GetNamespaceURI().wc_str());
    }
}

// (METHOD) WebDOMNode.getLocalName
// Description: Returns the local name
//
// Syntax: WebDOMNode.getLocalName : String
//
// Remarks: Returns the local name.
//
// Returns: The local name.

void WebDOMNode::getLocalName(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: review documentation

    retval->setNull();
    
    if (m_node.IsOk())
    {
        retval->setString(m_node.GetLocalName().wc_str());
    }
}

class WebEventListenerAdaptor : public wxEvtHandler
{
public:

    WebEventListenerAdaptor(Application* app, Event* evt)
    {
        m_evt = evt;
        m_evt->baseRef();
        
        m_app.setApp(app);
    }
    
    ~WebEventListenerAdaptor()
    {
        m_evt->baseUnref();
    }
    
    bool ProcessEvent(wxEvent& evt)
    {
        Application* app = m_app.getApp();
        if (app)
        {
            app->postEvent(m_evt, NULL);
        }

        return true;
    }
    
public:

    ApplicationWeakReference m_app;
    Event* m_evt;
};


static Application* getApplicationFromEnv(kscript::ExprEnv* env)
{
    // get an Application pointer 
    Application* app = NULL;

    kscript::ExprParser* parser = env->getParser();
    wxASSERT_MSG(parser != NULL, wxT("parser != NULL"));
    
    kscript::Value* val = parser->getBindVariable(L"Application");
    wxASSERT_MSG(val != NULL, wxT("val != NULL"));
    
    app = (Application*)val->getObject();
    wxASSERT_MSG(app != NULL, wxT("app != NULL"));
    
    return app;
}


void WebDOMNode::addEventListener(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
    
    if (env->getParamCount() < 2)
        return;
    
    bool use_capture = false;
    if (env->getParamCount() >= 3)
        use_capture = env->getParam(2)->getBoolean();
    
    
    if (env->getParam(1)->isFunction())
    {
        Event* evt = Event::createObject(env);
        WebEventListenerAdaptor* adaptor = new WebEventListenerAdaptor(getApplicationFromEnv(env), evt);
        evt->connectInternal(env->getParam(1));
        
        m_node.AddEventListener(env->getParam(0)->getString(), adaptor, -1, use_capture);
    }
     else if (env->getParam(1)->isObject())
    {
        kscript::ValueObject* obj = env->getParam(1)->getObject();
        if (obj->isKindOf(L"Event"))
        {
            Event* evt = (Event*)obj;
            WebEventListenerAdaptor* adaptor = new WebEventListenerAdaptor(getApplicationFromEnv(env), evt);
            
            m_node.AddEventListener(env->getParam(0)->getString(), adaptor, -1, use_capture);
        }
         else
        {
            retval->setBoolean(false);
            return;
        }
    }

}



// (CLASS) WebDOMText
// Category: DOM
// Derives: WebDOMNode
// Description: A class that represents a DOM text node.
// Remarks: The WebDOMText class represents a DOM text node.

WebDOMText::WebDOMText()
{
}

WebDOMText::~WebDOMText()
{
}

// (CONSTRUCTOR) WebDOMText.constructor
//
// Description: Creates a new WebDOMText object.
//
// Syntax: WebDOMText()
//
// Remarks: Creates a new WebDOMText object.

void WebDOMText::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

// (METHOD) WebDOMText.setData
// Description: Sets the data for the text node.
//
// Syntax: WebDOMText.setData(text : String) : Boolean
//
// Remarks: Sets the data for the text node. Returns true if the data was
//     set, and false otherwise.
//
// Param(text): The |text| to which to set the data of the text node.
//
// Returns: True if the data was set, and false otherwise.

void WebDOMText::setData(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
    
    if (!m_node.IsOk())
        return;
    
    if (env->getParamCount() < 1)
        return;
    
    wxDOMText node = m_node;
    node.SetData(env->getParam(0)->getString());
    
    retval->setBoolean(true);
}

// (METHOD) WebDOMText.getData
// Description: Gets the data of the text node.
//
// Syntax: WebDOMText.getData() : String
//
// Remarks: Gets the data of the text node.
//
// Returns: The data of the text node.

void WebDOMText::getData(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();

    wxDOMText node = m_node;
    if (node.IsOk())
    {
        retval->setString(node.GetData().wc_str());
    }
}

// (CLASS) WebDOMAttr
// Category: DOM
// Derives: WebDOMNode
// Description: A class that represents a DOM attribute node.
// Remarks: The WebDOMAttr class represents a DOM attribute node.

WebDOMAttr::WebDOMAttr()
{
}

WebDOMAttr::~WebDOMAttr()
{
}

// (CONSTRUCTOR) WebDOMAttr.constructor
//
// Description: Creates a new WebDOMAttr object.
//
// Syntax: WebDOMAttr()
//
// Remarks: Creates a new WebDOMAttr object.

void WebDOMAttr::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

// (METHOD) WebDOMAttr.getName
// Description: Gets the name of this attribute node.
//
// Syntax: WebDOMAttr.getName() : String
//
// Remarks: Returns the name of this attribute node.
//
// Returns: Returns the name of this attribute node.

void WebDOMAttr::getName(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: review documentation

    retval->setNull();

    wxDOMAttr attr = m_node;
    if (attr.IsOk())
    {
        retval->setString(attr.GetName().wc_str());
    }
}

// (METHOD) WebDOMAttr.getSpecified
// Description:
//
// Syntax: WebDOMAttr.getSpecified() : Boolean
//
// Remarks:
//
// Returns:

void WebDOMAttr::getSpecified(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: document

    retval->setBoolean(false);

    wxDOMAttr attr = m_node;
    if (attr.IsOk())
    {
        retval->setBoolean(attr.GetSpecified());
    }
}

// (METHOD) WebDOMAttr.getValue
// Description: Gets the value of this attribute node.
//
// Syntax: WebDOMAttr.getValue() : String
//
// Remarks: Returns the value of this attribute node.
//
// Returns: Returns the value of this attribute node.

void WebDOMAttr::getValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: review documentation

    retval->setNull();

    wxDOMAttr attr = m_node;
    if (attr.IsOk())
    {
        retval->setString(attr.GetValue().wc_str());
    }
}

// (METHOD) WebDOMAttr.getOwnerElement
// Description: Gets the element to which the node belongs.
//
// Syntax: WebDOMAttr.getOwnerElement() : wxDOMElement
//
// Remarks: Returns a WebDOMElement object that represents 
//     the element to which the node belongs.
//
// Returns: A WebDOMElement object that represents the element
//     to which the node belongs.

void WebDOMAttr::getOwnerElement(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: review documentation

    retval->setNull();

    wxDOMAttr attr = m_node;
    if (!attr.IsOk())
        return;

    wxDOMElement result;
    
    result = attr.GetOwnerElement();
    if (result.IsOk())
    {
        WebDOMNode* res = createAppropriateDOMNode(env, result);
        res->m_node = result;
        retval->setObject(res);
    }
}


// (CLASS) WebDOMElement
// Category: DOM
// Derives: WebDOMNode
// Description: A class that represents a DOM element.
// Remarks: The WebDOMElement class represents a DOM element.

WebDOMElement::WebDOMElement()
{
}

WebDOMElement::~WebDOMElement()
{
}

// (CONSTRUCTOR) WebDOMElement.constructor
//
// Description: Creates a new WebDOMElement object.
//
// Syntax: WebDOMElement()
//
// Remarks: Creates a new WebDOMElement object.

void WebDOMElement::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

// (METHOD) WebDOMElement.getTagName
// Description: Gets the tag name of this element.
//
// Syntax: WebDOMElement.getTagName() : String
//
// Remarks: Returns the tag name of this element.
//
// Returns: Returns the tag name of this element.

void WebDOMElement::getTagName(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();

    wxDOMElement element = m_node;
    if (element.IsOk())
    {
        retval->setString(element.GetTagName().wc_str());
    }
}


// (METHOD) WebDOMElement.getAttributes
// Description: Gets the attributes of this node.
//
// Syntax: WebDOMNode.getAttributes() : Array(WebDOMAttr)
//
// Remarks: Returns an array of attributes.
//
// Returns: An array of attributes.

void WebDOMElement::getAttributes(kscript::ExprEnv* env, kscript::Value* retval)
{
    // note: in the DOM specification, this returns a DOM 
    // Named Node Map; however, the practical way it appears
    // to be exposed in conventional ECMAScript is as an 
    // array of DOM attributes

    // set the default return value
    retval->setArray(env);

    wxDOMElement element = m_node;
    if (!element.IsOk())
        return;

    // get the attributes
    wxDOMNamedAttrMap attributes = element.GetAttributes();
    if (!attributes.IsOk())
        return;

    // add each attribute to the output arrays
    size_t i;
    size_t count = attributes.GetLength();
    for (i = 0; i < count; ++i)
    {
        wxDOMNode node = attributes.Item(i);
        if (node.IsOk())
        {
            WebDOMNode* attr = createAppropriateDOMNode(env, node);
            attr->m_node = node;
            
            kscript::Value val;
            val.setObject(attr);
            retval->appendMember(&val);
        }
    }
}

// (METHOD) WebDOMElement.getElementsByTagName
// Description: Gets an array of child elements having the specified tag name.
//
// Syntax: WebDOMElement.getElementsByTagName(tag_name : String) : Array(WebDOMElement)
//
// Remarks: Returns an array of all the child elements of this
//     element which have the specified tag name.
//
// Param(tag_name): The |tag_name| of the child elements that are
//     to be returned.
//
// Returns: Returns an array of all the child elements of this 
//     element which have the specified tag name.

void WebDOMElement::getElementsByTagName(kscript::ExprEnv* env, kscript::Value* retval)
{
    // set the default return value
    retval->setArray(env);

    // if the current node isn't an element, we're done
    wxDOMElement element = m_node;
    if (!element.IsOk())
        return;

    // if no input string is specified, we're done
    if (env->getParamCount() < 1)
        return;

    wxDOMHTMLCollection child_nodes;
    wxString tag_name = env->getParam(0)->getString();
    child_nodes = element.GetElementsByTagName(tag_name);

    if (!child_nodes.IsOk())
        return;
    
    size_t count = child_nodes.GetLength();
    for (size_t i = 0; i < count; ++i)
    {
        WebDOMElement* child_element = WebDOMElement::createObject(env);
        child_element->m_node = child_nodes.Item(i);
        
        kscript::Value val;
        val.setObject(child_element);
        retval->appendMember(&val);
    }
}

// (METHOD) WebDOMElement.getAttribute
// Description: Gets the attribute of this element.
//
// Syntax: WebDOMElement.getAttribute(attr : String) : String
//
// Remarks: Returns the value of the specified attribute of this element.
//
// Param(attr): The attribute for which to return the value.
//
// Returns: Returns the value of the specified attribute of this element.

void WebDOMElement::getAttribute(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();

    wxDOMElement element = m_node;
    if (element.IsOk() && env->getParamCount() > 0)
    {
        wxString value = env->getParam(0)->getString();
        retval->setString(element.GetAttribute(value).wc_str());
    }
}

// (METHOD) WebDOMElement.setAttribute
// Description: Sets the attribute of this element to a specified value.
//
// Syntax: WebDOMElement.setAttribute(attr : String,
//                                    value : String)
//
// Remarks: Sets the |attr| attribute of this element to |value|.
//
// Param(attr): The attribute to set.
// Param(value): The |value| to which to set the attribute.

void WebDOMElement::setAttribute(kscript::ExprEnv* env, kscript::Value* retval)
{
    wxDOMElement element = m_node;
    if (element.IsOk() && env->getParamCount() > 0)
    {
        wxString value;
        if (env->getParamCount() > 1)
            value = env->getParam(1)->getString();
            
        element.SetAttribute(env->getParam(0)->getString(), value);
    }
}

// (METHOD) WebDOMElement.removeAttribute
// Description: Removes the specified attribute from this element.
//
// Syntax: WebDOMElement.removeAttribute(attr : String)
//
// Remarks: Removes the specified attribute from this element.
//
// Param(attr): The attribute to remove.

void WebDOMElement::removeAttribute(kscript::ExprEnv* env, kscript::Value* retval)
{
    wxDOMElement element = m_node;
    if (element.IsOk() && env->getParamCount() > 0)
    {
        wxString value = env->getParam(0)->getString();
        element.RemoveAttribute(value);
    }
}

// (METHOD) WebDOMElement.getAttributeNode
// Description: Gets the attribute node of this element.
//
// Syntax: WebDOMElement.getAttributeNode(attr : String) : WebDOMAttr
//
// Remarks:
//
// Param(attr):
//
// Returns:

void WebDOMElement::getAttributeNode(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: document

    retval->setNull();
    
    if (env->getParamCount() < 1)
        return;
        
    wxDOMElement element;
    element = m_node;
    if (!element.IsOk())
        return;
      
    wxDOMAttr result = element.GetAttributeNode(env->getParam(0)->getString());
    if (result.IsOk())
    {
        WebDOMAttr* res = WebDOMAttr::createObject(env);
        res->m_node = result;
        retval->setObject(res);
    }
}

// (METHOD) WebDOMElement.setAttributeNode
// Description: Sets the attribute node of this element.
//
// Syntax: WebDOMElement.setAttributeNode(attr : WebDOMAttr) : WebDOMAttr
//
// Remarks:
//
// Param(attr):
//
// Returns:

void WebDOMElement::setAttributeNode(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: document

    retval->setNull();

    wxDOMElement element;
    element = m_node;
    if (!element.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    if (!env->getParam(0)->isObject())
        return;

    kscript::ValueObject* vobj = env->getParam(0)->getObject();
    if (!vobj->isKindOf(L"WebDOMAttr"))
        return;

    WebDOMAttr* attrobj = (WebDOMAttr*)vobj;
    wxDOMAttr attr = attrobj->m_node;

    wxDOMAttr result;
    result = element.SetAttributeNode(attr);
    if (result.IsOk())
    {
        WebDOMAttr* res = WebDOMAttr::createObject(env);
        res->m_node = result;
        retval->setObject(res);
    }
}

// (METHOD) WebDOMElement.removeAttributeNode
// Description: Removes the attribute node of this element.
//
// Syntax: WebDOMElement.removeAttributeNode(attr : WebDOMAttr) : WebDOMAttr
//
// Remarks:
//
// Param(attr):
//
// Returns:

void WebDOMElement::removeAttributeNode(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: document

    retval->setNull();

    wxDOMElement element;
    element = m_node;
    if (!element.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    if (!env->getParam(0)->isObject())
        return;

    kscript::ValueObject* vobj = env->getParam(0)->getObject();
    if (!vobj->isKindOf(L"WebDOMAttr"))
        return;

    WebDOMAttr* attrobj = (WebDOMAttr*)vobj;
    wxDOMAttr attr = attrobj->m_node;

    wxDOMAttr result;
    result = element.RemoveAttributeNode(attr);
    if (result.IsOk())
    {
        WebDOMAttr* res = WebDOMAttr::createObject(env);
        res->m_node = result;
        retval->setObject(res);
    }
}

// (METHOD) WebDOMElement.hasAttribute
// Description: Returns true if the specified attribute exists, 
//     and false otherwise.
//
// Syntax: WebDOMElement.hasAttribute(attr : String)
//
// Remarks: Returns true if the specified attribute exists in the element, 
//     and false otherwise.
//
// Param(attr): The attribute to determine if exists in the element.
//
// Returns: Returns true if the specified attribute exists, and false otherwise.

void WebDOMElement::hasAttribute(kscript::ExprEnv* env, kscript::Value* retval)
{
    wxDOMElement element = m_node;
    if (element.IsOk() && env->getParamCount() > 0)
    {
        wxString value = env->getParam(0)->getString();
        retval->setBoolean(element.HasAttribute(value));
    }
}

// (CLASS) WebDOMDocument
// Category: DOM
// Derives: WebDOMNode
// Description: A class that represents a DOM document.
// Remarks: The WebDOMNode class represents a DOM document.

WebDOMDocument::WebDOMDocument()
{
}

WebDOMDocument::~WebDOMDocument()
{
}

// (CONSTRUCTOR) WebDOMDocument.constructor
//
// Description: Creates a new WebDOMDocument object.
//
// Syntax: WebDOMDocument()
//
// Remarks: Creates a new WebDOMDocument object.

void WebDOMDocument::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

// (METHOD) WebDOMDocument.getDocumentElement
// Description: Gets the document element.
//
// Syntax: WebDOMDocument.getDocumentElement() : wxDOMElement
//
// Remarks: Returns the document element.
//
// Returns: Returns the document element.

void WebDOMDocument::getDocumentElement(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation

    retval->setNull();

    wxDOMDocument doc = m_node;
    if (!doc.IsOk())
        return;

    wxDOMElement result;
    result = doc.GetDocumentElement();
    if (result.IsOk())
    {
        WebDOMNode* res = createAppropriateDOMNode(env, result);
        res->m_node = result;
        retval->setObject(res);
    }
}

// (METHOD) WebDOMDocument.getElementById
// Description: Gets the element with a particular id from a document.
//
// Syntax: WebDOMDocument.getElementById(id : String) : WebDOMElement
//
// Remarks: Returns the element with the specified |id| from the document.
//
// Param(id): The |id| of the child element to return.
//
// Returns:  Returns the element with the specified |id| from the document.

void WebDOMDocument::getElementById(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();
    
    if (env->getParamCount() < 1)
        return;
        
    wxDOMDocument doc;
    doc = m_node;
    if (!doc.IsOk())
        return;
      
    wxDOMElement result = doc.GetElementById(env->getParam(0)->getString());
    if (result.IsOk())
    {
        WebDOMNode* res = createAppropriateDOMNode(env, result);
        res->m_node = result;
        retval->setObject(res);
    }
}

// (METHOD) WebDOMDocument.getElementsByTagName
// Description: Gets an array of child elements having the specified tag name.
//
// Syntax: WebDOMDocument.getElementsByTagName(tag_name : String) : Array(WebDOMElement)
//
// Remarks: Returns an array of all the child elements of this
//     document which have the specified tag name.
//
// Param(tag_name): The |tag_name| of the child elements that are
//     to be returned.
//
// Returns: Returns an array of all the child elements of this 
//     document which have the specified tag name.

void WebDOMDocument::getElementsByTagName(kscript::ExprEnv* env, kscript::Value* retval)
{
    // set the default return value
    retval->setArray(env);

    // if the current node isn't an element, we're done
    wxDOMDocument doc = m_node;
    if (!doc.IsOk())
        return;

    // if no input string is specified, we're done
    if (env->getParamCount() < 1)
        return;

    wxDOMNodeList child_nodes;
    wxString tag_name = env->getParam(0)->getString();
    child_nodes = doc.GetElementsByTagName(tag_name);

    if (!child_nodes.IsOk())
        return;
    
    size_t count = child_nodes.GetLength();
    for (size_t i = 0; i < count; ++i)
    {
        wxDOMNode node = child_nodes.Item(i);
        
        WebDOMNode* child_element = createAppropriateDOMNode(env, node);
        child_element->m_node = node;
        
        kscript::Value val;
        val.setObject(child_element);
        retval->appendMember(&val);
    }
}

// (METHOD) WebDOMDocument.importNode
// Description: Imports a node into this document.
//
// Syntax: WebDOMDocument.importNode(node : WebDOMNode,
//                                   deep : Boolean) : WebDOMNode
//
// Remarks: Imports a node into this document.  If the deep parameter
//     is true, then all the child nodes of the imported node are also
//     imported.
//
// Param(node): A |node| to import.
// Param(deep): Boolean value that specifies whether or not to import
//     all the child nodes of the imported node in addition to the node
//     itself.
//
// Returns: A copy of the imported node.

void WebDOMDocument::importNode(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();

    wxDOMDocument doc = m_node;
    if (!doc.IsOk())
        return;
    
    if (env->getParamCount() < 1)
        return;
        
    if (!env->getParam(0)->isObject())
        return;

    kscript::ValueObject* vobj = env->getParam(0)->getObject();
    if (!vobj->isKindOf(L"WebDOMNode"))
        return;

    // get the deep flag
    bool deep = false;
    if (env->getParamCount() > 1)
        deep = env->getParam(1)->getBoolean();

    // import the node
    WebDOMNode* child_node = (WebDOMNode*)vobj;

    wxDOMNode result;
    result = doc.ImportNode(child_node->m_node, deep);
    if (result.IsOk())
    {
        WebDOMNode* res = createAppropriateDOMNode(env, result);
        res->m_node = result;
        retval->setObject(res);
    }
}

// (METHOD) WebDOMDocument.createElement
// Description: Creates an element.
//
// Syntax: WebDOMDocument.createElement(tag_name : String) : WebDOMElement
//
// Remarks: Creates an element.
//
// Param(tag_name): The |tag_name| of the element to create.
//
// Returns: Returns the newly created element.


void WebDOMDocument::createElement(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();
    
    if (env->getParamCount() < 1)
        return;
    
    wxDOMDocument doc;
    doc = m_node;
    if (!doc.IsOk())
        return;

    wxDOMElement result = doc.CreateElement(env->getParam(0)->getString());
    if (result.IsOk())
    {
        WebDOMNode* res = createAppropriateDOMNode(env, result);
        res->m_node = result;
        retval->setObject(res);
    }
}

// (METHOD) WebDOMDocument.createAttribute
// Description: Creates an attribute.
//
// Syntax: WebDOMDocument.createAttribute(name : String) : WebDOMAttr
//
// Remarks: Creates an attribute.
//
// Param(tag_name): The |tag_name| of the attribute to create.
//
// Returns: Returns the newly created attribute.

void WebDOMDocument::createAttribute(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();

    if (env->getParamCount() < 1)
        return;
    
    wxDOMDocument doc;
    doc = m_node;
    if (!doc.IsOk())
        return;

    wxDOMAttr result = doc.CreateElement(env->getParam(0)->getString());
    if (result.IsOk())
    {
        WebDOMAttr* res = WebDOMAttr::createObject(env);
        res->m_node = result;
        retval->setObject(res);
    }
}

// (METHOD) WebDOMDocument.createTextNode
// Description: Creates a text node.
//
// Syntax: WebDOMDocument.createTextNode(name : String) : WebDOMAttr
//
// Remarks: Creates a text node.
//
// Param(tag_name): The |tag_name| of the text node to create.
//
// Returns: Returns the newly created text node.

void WebDOMDocument::createTextNode(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();
    
    if (env->getParamCount() < 1)
        return;
    
    wxDOMDocument doc;
    doc = m_node;
    if (!doc.IsOk())
        return;

    wxDOMText result = doc.CreateTextNode(env->getParam(0)->getString());
    if (result.IsOk())
    {
        WebDOMText* res = WebDOMText::createObject(env);
        res->m_node = result;
        retval->setObject(res);
    }
}


// (CLASS) WebDOMHTMLElement
// Category: DOM
// Derives: WebDOMElement
// Description: A class that represents a DOM html element.
// Remarks: The WebDOMHTMLElement class represents a DOM html element.

WebDOMHTMLElement::WebDOMHTMLElement()
{
}

WebDOMHTMLElement::~WebDOMHTMLElement()
{
}

// (CONSTRUCTOR) WebDOMHTMLElement.constructor
//
// Description: Creates a new WebDOMHTMLElement object.
//
// Syntax: WebDOMHTMLElement()
//
// Remarks: Creates a new WebDOMHTMLElement object.

void WebDOMHTMLElement::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

// (METHOD) WebDOMHTMLElement.getId
// Description: Gets the element's id.
//
// Syntax: WebDOMHTMLElement.getId() : String
//
// Remarks: Get's the element's id.
//
// Returns: Returns a string that is the element's id.

void WebDOMHTMLElement::getId(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetId();

    retval->setNull();
    
    wxDOMHTMLElement e = m_node;
    if (!e.IsOk())
        return;
        
    retval->setString(e.GetId().wc_str());
}

// (METHOD) WebDOMHTMLElement.setId
// Description: Sets the element's id.
//
// Syntax: WebDOMHTMLElement.setId(value : String)
//
// Remarks: Set's the element's id.
// Param(value): The |value| to which to set the id.

void WebDOMHTMLElement::setId(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetId(const wxString& value);

    wxDOMHTMLElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetId(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLElement.getTitle
// Description: Gets the element's title.
//
// Syntax: WebDOMHTMLElement.getTitle() : String
//
// Remarks: Get's the element's title.
//
// Returns: Returns a string that is the element's title.

void WebDOMHTMLElement::getTitle(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetTitle();

    retval->setNull();

    wxDOMHTMLElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetTitle().wc_str());
}

// (METHOD) WebDOMHTMLElement.setTitle
// Description: Sets the element's title
//
// Syntax: WebDOMHTMLElement.setTitle(value : String)
//
// Remarks: Set's the element's title.
// Param(value): The |value| to which to set the title.

void WebDOMHTMLElement::setTitle(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetTitle(const wxString& value);
    
    wxDOMHTMLElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetTitle(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLElement.getLang
// Description:
//
// Syntax: WebDOMHTMLElement.getLang() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLElement::getLang(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetLang();

    retval->setNull();

    wxDOMHTMLElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetLang().wc_str());
}

// (METHOD) WebDOMHTMLElement.setLang
// Description:
//
// Syntax: WebDOMHTMLElement.setLang(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLElement::setLang(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetLang(const wxString& value);

    wxDOMHTMLElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetLang(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLElement.getDir
// Description:
//
// Syntax: WebDOMHTMLElement.getDir() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLElement::getDir(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetDir();

    retval->setNull();

    wxDOMHTMLElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetDir().wc_str());
}

// (METHOD) WebDOMHTMLElement.setDir
// Description:
//
// Syntax: WebDOMHTMLElement.setDir(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLElement::setDir(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetDir(const wxString& value);

    wxDOMHTMLElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetDir(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLElement.getClassName
// Description:
//
// Syntax: WebDOMHTMLElement.getClassName() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLElement::getClassName(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetClassName();

    retval->setNull();

    wxDOMHTMLElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetClassName().wc_str());
}

// (METHOD) WebDOMHTMLElement.setClassName
// Description:
//
// Syntax: WebDOMHTMLElement.setClassName(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLElement::setClassName(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetClassName(const wxString& value);
    
    wxDOMHTMLElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetClassName(env->getParam(0)->getString());
}


// (METHOD) WebDOMHTMLElement.getValue
// Description: Returns the current value of the node.
//
// Syntax: WebDOMHTMLElement.getValue() : String
//
// Remarks: Returns the current value of the node. If the node 
//     corresponds to a form input, the value is the one currently
//     entered in the form.
//
// Returns: Returns the current value of the node. If the node 
//     corresponds to a form input, the value is the one currently
//     entered in the form.

void WebDOMHTMLElement::getValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();
    
    wxDOMHTMLElement e = m_node;
    if (!e.IsOk() || !e.HasValueProperty())
        return;
        
    retval->setString(e.GetValue().wc_str());
}

// (METHOD) WebDOMHTMLElement.setValue
// Description: Sets the current value of the node.
//
// Syntax: WebDOMHTMLElement.setValue(text : String)
//
// Remarks: Sets the current value of the node. If the node 
//     corresponds to a form input, the value is the one 
//     that will actually show in the form.
//
// Param(text): The value to which to set the current value
//     of the node. If the node corresponds to a form input, 
//     the value is the one that will actually show in the form.

void WebDOMHTMLElement::setValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);

    wxDOMHTMLElement e = m_node;
    if (!e.IsOk() || !e.HasValueProperty())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetValue(env->getParam(0)->getString());

    retval->setBoolean(true);
}



// (METHOD) WebDOMHTMLElement.getTabIndex
// Description:
//
// Syntax: WebDOMHTMLElement.getTabIndex() : Integer
//
// Remarks:
//
// Returns:

void WebDOMHTMLElement::getTabIndex(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // int GetTabIndex();
    
    retval->setNull();

    wxDOMHTMLElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setInteger(e.GetTabIndex());
}

// (METHOD) WebDOMHTMLElement.setTabIndex
// Description:
//
// Syntax: WebDOMHTMLElement.setTabIndex(value : Integer)
//
// Remarks:
// Param(value):

void WebDOMHTMLElement::setTabIndex(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetTabIndex(int index);
    
    wxDOMHTMLElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetTabIndex(env->getParam(0)->getInteger());
}


// (METHOD) WebDOMHTMLElement.getAccessKey
// Description:
//
// Syntax: WebDOMHTMLElement.getAccessKey() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLElement::getAccessKey(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetAccessKey();
    
    retval->setNull();

    wxDOMHTMLElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetAccessKey().wc_str());
}

// (METHOD) WebDOMHTMLElement.setAccessKey
// Description:
//
// Syntax: WebDOMHTMLElement.setAccessKey(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLElement::setAccessKey(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetAccessKey(const wxString& value);
    
    wxDOMHTMLElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetAccessKey(env->getParam(0)->getString());
}


// (CLASS) WebDOMHTMLAnchorElement
// Category: DOM
// Derives: WebDOMHTMLElement
// Description: A class that represents an HTML anchor element.
// Remarks: The WebDOMHTMLAnchorElement class represents an HTML anchor element.

WebDOMHTMLAnchorElement::WebDOMHTMLAnchorElement()
{
}

WebDOMHTMLAnchorElement::~WebDOMHTMLAnchorElement()
{
}

// (CONSTRUCTOR) WebDOMHTMLAnchorElement.constructor
//
// Description: Creates a new WebDOMHTMLAnchorElement object.
//
// Syntax: WebDOMHTMLAnchorElement()
//
// Remarks: Creates a new WebDOMHTMLAnchorElement object.

void WebDOMHTMLAnchorElement::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}


// (METHOD) WebDOMHTMLAnchorElement.getCharset
// Description:
//
// Syntax: WebDOMHTMLAnchorElement.getCharset() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLAnchorElement::getCharset(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetCharset();

    retval->setNull();

    wxDOMHTMLAnchorElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetCharset().wc_str());
}

// (METHOD) WebDOMHTMLAnchorElement.setCharset
// Description:
//
// Syntax: WebDOMHTMLAnchorElement.setCharset(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLAnchorElement::setCharset(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetCharset(const wxString& value);

    wxDOMHTMLAnchorElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetCharset(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLAnchorElement.getCoords
// Description:
//
// Syntax: WebDOMHTMLAnchorElement.getCoords() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLAnchorElement::getCoords(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetCoords();

    retval->setNull();

    wxDOMHTMLAnchorElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetCoords().wc_str());
}

// (METHOD) WebDOMHTMLAnchorElement.setCoords
// Description:
//
// Syntax: WebDOMHTMLAnchorElement.setCoords(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLAnchorElement::setCoords(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetCoords(const wxString& value);

    wxDOMHTMLAnchorElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetCoords(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLAnchorElement.getHref
// Description:
//
// Syntax: WebDOMHTMLAnchorElement.getHref() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLAnchorElement::getHref(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetHref();
    
    retval->setNull();

    wxDOMHTMLAnchorElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetHref().wc_str());
}

// (METHOD) WebDOMHTMLAnchorElement.setHref
// Description:
//
// Syntax: WebDOMHTMLAnchorElement.setHref(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLAnchorElement::setHref(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetHref(const wxString& value);

    wxDOMHTMLAnchorElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetHref(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLAnchorElement.getHreflang
// Description:
//
// Syntax: WebDOMHTMLAnchorElement.getHreflang() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLAnchorElement::getHreflang(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetHreflang();

    retval->setNull();

    wxDOMHTMLAnchorElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetHreflang().wc_str());
}

// (METHOD) WebDOMHTMLAnchorElement.setHreflang
// Description:
//
// Syntax: WebDOMHTMLAnchorElement.setHreflang(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLAnchorElement::setHreflang(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetHreflang(const wxString& value);
    
    wxDOMHTMLAnchorElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetHreflang(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLAnchorElement.getName
// Description:
//
// Syntax: WebDOMHTMLAnchorElement.getName() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLAnchorElement::getName(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetName();

    retval->setNull();

    wxDOMHTMLAnchorElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetName().wc_str());
}

// (METHOD) WebDOMHTMLAnchorElement.setName
// Description:
//
// Syntax: WebDOMHTMLAnchorElement.setName(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLAnchorElement::setName(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetName(const wxString& value);

    wxDOMHTMLAnchorElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetName(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLAnchorElement.getRel
// Description:
//
// Syntax: WebDOMHTMLAnchorElement.getRel() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLAnchorElement::getRel(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetRel();

    retval->setNull();

    wxDOMHTMLAnchorElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetRel().wc_str());
}

// (METHOD) WebDOMHTMLAnchorElement.setRel
// Description:
//
// Syntax: WebDOMHTMLAnchorElement.setRel(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLAnchorElement::setRel(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetRel(const wxString& value);

    wxDOMHTMLAnchorElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetRel(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLAnchorElement.getRev
// Description:
//
// Syntax: WebDOMHTMLAnchorElement.getRev() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLAnchorElement::getRev(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetRev();

    retval->setNull();

    wxDOMHTMLAnchorElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetRev().wc_str());
}

// (METHOD) WebDOMHTMLAnchorElement.setRev
// Description:
//
// Syntax: WebDOMHTMLAnchorElement.setRev(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLAnchorElement::setRev(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetRev(const wxString& value);

    wxDOMHTMLAnchorElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetRev(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLAnchorElement.getShape
// Description:
//
// Syntax: WebDOMHTMLAnchorElement.getShape() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLAnchorElement::getShape(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetShape();

    retval->setNull();

    wxDOMHTMLAnchorElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetShape().wc_str());
}

// (METHOD) WebDOMHTMLAnchorElement.setShape
// Description:
//
// Syntax: WebDOMHTMLAnchorElement.setShape(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLAnchorElement::setShape(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetShape(const wxString& value);

    wxDOMHTMLAnchorElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetShape(env->getParam(0)->getString());
}


// (METHOD) WebDOMHTMLAnchorElement.getTarget
// Description:
//
// Syntax: WebDOMHTMLAnchorElement.getTarget() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLAnchorElement::getTarget(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetTarget();
    
    retval->setNull();

    wxDOMHTMLAnchorElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetTarget().wc_str());
}

// (METHOD) WebDOMHTMLAnchorElement.setTarget
// Description:
//
// Syntax: WebDOMHTMLAnchorElement.setTarget(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLAnchorElement::setTarget(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetTarget(const wxString& value);
    
    wxDOMHTMLAnchorElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetTarget(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLAnchorElement.getType
// Description:
//
// Syntax: WebDOMHTMLAnchorElement.getType() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLAnchorElement::getType(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetType();

    retval->setNull();

    wxDOMHTMLAnchorElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetType().wc_str());
}

// (METHOD) WebDOMHTMLAnchorElement.setType
// Description:
//
// Syntax: WebDOMHTMLAnchorElement.setType(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLAnchorElement::setType(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetType(const wxString& value);
    
    wxDOMHTMLAnchorElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetType(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLAnchorElement.blur
// Description:
//
// Syntax: WebDOMHTMLAnchorElement.blur()
//
// Remarks:

void WebDOMHTMLAnchorElement::blur(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void Blur();
    
    wxDOMHTMLAnchorElement e = m_node;
    if (!e.IsOk())
        return;

    e.Blur();
}

// (METHOD) WebDOMHTMLAnchorElement.focus
// Description:
//
// Syntax: WebDOMHTMLAnchorElement.focus()
//
// Remarks:

void WebDOMHTMLAnchorElement::focus(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void Focus();
    
    wxDOMHTMLAnchorElement e = m_node;
    if (!e.IsOk())
        return;

    e.Focus();
}

// (CLASS) WebDOMHTMLButtonElement
// Category: DOM
// Derives: WebDOMHTMLElement
// Description: A class that represents an HTML button element.
// Remarks: The WebDOMHTMLButtonElement class represents an HTML button element.

WebDOMHTMLButtonElement::WebDOMHTMLButtonElement()
{
}

WebDOMHTMLButtonElement::~WebDOMHTMLButtonElement()
{
}

// (CONSTRUCTOR) WebDOMHTMLButtonElement.constructor
//
// Description: Creates a new WebDOMHTMLButtonElement object.
//
// Syntax: WebDOMHTMLButtonElement()
//
// Remarks: Creates a new WebDOMHTMLButtonElement object.

void WebDOMHTMLButtonElement::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}


// (METHOD) WebDOMHTMLButtonElement.getDisabled
// Description:
//
// Syntax: WebDOMHTMLButtonElement.getDisabled() : Boolean
//
// Remarks:
//
// Returns:

void WebDOMHTMLButtonElement::getDisabled(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // bool GetDisabled();

    retval->setNull();

    wxDOMHTMLButtonElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setBoolean(e.GetDisabled());
}

// (METHOD) WebDOMHTMLButtonElement.setDisabled
// Description:
//
// Syntax: WebDOMHTMLButtonElement.setDisabled(value : Boolean)
//
// Remarks:
// Param(value):

void WebDOMHTMLButtonElement::setDisabled(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetDisabled(bool value);

    wxDOMHTMLButtonElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetDisabled(env->getParam(0)->getBoolean());
}

// (METHOD) WebDOMHTMLButtonElement.getName
// Description:
//
// Syntax: WebDOMHTMLButtonElement.getName() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLButtonElement::getName(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetName();

    retval->setNull();

    wxDOMHTMLButtonElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetName().wc_str());
}

// (METHOD) WebDOMHTMLButtonElement.setName
// Description:
//
// Syntax: WebDOMHTMLButtonElement.setName(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLButtonElement::setName(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetName(const wxString& value);

    wxDOMHTMLButtonElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetName(env->getParam(0)->getString());
}


// (METHOD) WebDOMHTMLButtonElement.getType
// Description:
//
// Syntax: WebDOMHTMLButtonElement.getType() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLButtonElement::getType(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetType();

    retval->setNull();

    wxDOMHTMLButtonElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetType().wc_str());
}

// (CLASS) WebDOMHTMLInputElement
// Category: DOM
// Derives: WebDOMHTMLElement
// Description: A class that represents an HTML input element.
// Remarks: The WebDOMHTMLInputElement class represents an HTML input element.

WebDOMHTMLInputElement::WebDOMHTMLInputElement()
{
}

WebDOMHTMLInputElement::~WebDOMHTMLInputElement()
{
}

// (CONSTRUCTOR) WebDOMHTMLInputElement.constructor
//
// Description: Creates a new WebDOMHTMLInputElement object.
//
// Syntax: WebDOMHTMLInputElement()
//
// Remarks: Creates a new WebDOMHTMLInputElement object.

void WebDOMHTMLInputElement::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

// (METHOD) WebDOMHTMLInputElement.getDefaultValue
// Description:
//
// Syntax: WebDOMHTMLInputElement.getDefaultValue() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLInputElement::getDefaultValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetDefaultValue();

    retval->setNull();

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetDefaultValue().wc_str());
}

// (METHOD) WebDOMHTMLInputElement.setDefaultValue
// Description:
//
// Syntax: WebDOMHTMLInputElement.setDefaultValue(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLInputElement::setDefaultValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetDefaultValue(const wxString& value);

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetDefaultValue(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLInputElement.getDefaultChecked
// Description:
//
// Syntax: WebDOMHTMLInputElement.getDefaultChecked() : Boolean
//
// Remarks:
//
// Returns:

void WebDOMHTMLInputElement::getDefaultChecked(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // bool GetDefaultChecked();

    retval->setNull();

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setBoolean(e.GetDefaultChecked());
}

// (METHOD) WebDOMHTMLInputElement.setDefaultChecked
// Description:
//
// Syntax: WebDOMHTMLInputElement.setDefaultChecked(value : Boolean)
//
// Remarks:
// Param(value):

void WebDOMHTMLInputElement::setDefaultChecked(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetDefaultChecked(bool value);
    
    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetDefaultChecked(env->getParam(0)->getBoolean());
}

// (METHOD) WebDOMHTMLInputElement.getAccept
// Description:
//
// Syntax: WebDOMHTMLInputElement.getAccept() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLInputElement::getAccept(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetAccept();

    retval->setNull();

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetAccept().wc_str());
}

// (METHOD) WebDOMHTMLInputElement.setAccept
// Description:
//
// Syntax: WebDOMHTMLInputElement.setAccept(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLInputElement::setAccept(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetAccept(const wxString& value);

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetAccept(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLInputElement.getAlign
// Description:
//
// Syntax: WebDOMHTMLInputElement.getAlign() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLInputElement::getAlign(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetAlign();

    retval->setNull();

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetAlign().wc_str());
}

// (METHOD) WebDOMHTMLInputElement.setAlign
// Description:
//
// Syntax: WebDOMHTMLInputElement.setAlign(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLInputElement::setAlign(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetAlign(const wxString& value);

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetAlign(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLInputElement.getAlt
// Description:
//
// Syntax: WebDOMHTMLInputElement.getAlt() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLInputElement::getAlt(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetAlt();

    retval->setNull();

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetAlt().wc_str());
}

// (METHOD) WebDOMHTMLInputElement.setAlt
// Description:
//
// Syntax: WebDOMHTMLInputElement.setAlt(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLInputElement::setAlt(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetAlt(const wxString& value);

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetAlt(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLInputElement.getChecked
// Description:
//
// Syntax: WebDOMHTMLInputElement.getChecked() : Boolean
//
// Remarks:
//
// Returns:

void WebDOMHTMLInputElement::getChecked(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // bool GetChecked();

    retval->setNull();

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setBoolean(e.GetChecked());
}

// (METHOD) WebDOMHTMLInputElement.setChecked
// Description:
//
// Syntax: WebDOMHTMLInputElement.setChecked(value : Boolean)
//
// Remarks:
// Param(value):

void WebDOMHTMLInputElement::setChecked(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetChecked(bool value);

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetChecked(env->getParam(0)->getBoolean());
}

// (METHOD) WebDOMHTMLInputElement.getDisabled
// Description:
//
// Syntax: WebDOMHTMLInputElement.getDisabled() : Boolean
//
// Remarks:
//
// Returns:

void WebDOMHTMLInputElement::getDisabled(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // bool GetDisabled();

    retval->setNull();

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setBoolean(e.GetDisabled());
}

// (METHOD) WebDOMHTMLInputElement.setDisabled
// Description:
//
// Syntax: WebDOMHTMLInputElement.setDisabled(value : Boolean)
//
// Remarks:
// Param(value):

void WebDOMHTMLInputElement::setDisabled(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetDisabled(bool value);

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetDisabled(env->getParam(0)->getBoolean());
}

// (METHOD) WebDOMHTMLInputElement.getMaxLength
// Description:
//
// Syntax: WebDOMHTMLInputElement.getMaxLength() : Integer
//
// Remarks:
//
// Returns:

void WebDOMHTMLInputElement::getMaxLength(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // int GetMaxLength();
    
    retval->setNull();

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setInteger(e.GetMaxLength());
}

// (METHOD) WebDOMHTMLInputElement.setMaxLength
// Description:
//
// Syntax: WebDOMHTMLInputElement.setMaxLength(value : Integer)
//
// Remarks:
// Param(value):

void WebDOMHTMLInputElement::setMaxLength(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetMaxLength(int value);

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetMaxLength(env->getParam(0)->getInteger());
}

// (METHOD) WebDOMHTMLInputElement.getName
// Description:
//
// Syntax: WebDOMHTMLInputElement.getName() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLInputElement::getName(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetName();

    retval->setNull();

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetName().wc_str());
}

// (METHOD) WebDOMHTMLInputElement.setName
// Description:
//
// Syntax: WebDOMHTMLInputElement.setName(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLInputElement::setName(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetName(const wxString& value);

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetName(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLInputElement.getReadOnly
// Description:
//
// Syntax: WebDOMHTMLInputElement.getReadOnly() : Boolean
//
// Remarks:
//
// Returns:

void WebDOMHTMLInputElement::getReadOnly(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // bool GetReadOnly();
    
    retval->setNull();

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setBoolean(e.GetReadOnly());
}

// (METHOD) WebDOMHTMLInputElement.setReadOnly
// Description:
//
// Syntax: WebDOMHTMLInputElement.setReadOnly(value : Boolean)
//
// Remarks:
// Param(value):

void WebDOMHTMLInputElement::setReadOnly(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetReadOnly(bool value);

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetReadOnly(env->getParam(0)->getBoolean());
}

// (METHOD) WebDOMHTMLInputElement.getSize
// Description:
//
// Syntax: WebDOMHTMLInputElement.getSize() : Integer
//
// Remarks:
//
// Returns:

void WebDOMHTMLInputElement::getSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // int GetSize();
    
    retval->setNull();

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setInteger(e.GetSize());
}

// (METHOD) WebDOMHTMLInputElement.setSize
// Description:
//
// Syntax: WebDOMHTMLInputElement.setSize(value : Integer)
//
// Remarks:
// Param(value):

void WebDOMHTMLInputElement::setSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetSize(int value);

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetSize(env->getParam(0)->getInteger());
}

// (METHOD) WebDOMHTMLInputElement.getSrc
// Description:
//
// Syntax: WebDOMHTMLInputElement.getSrc() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLInputElement::getSrc(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetSrc();

    retval->setNull();

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetSrc().wc_str());
}

// (METHOD) WebDOMHTMLInputElement.setSrc
// Description:
//
// Syntax: WebDOMHTMLInputElement.setSrc(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLInputElement::setSrc(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetSrc(const wxString& value);

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetSrc(env->getParam(0)->getString());
}


// (METHOD) WebDOMHTMLInputElement.getType
// Description:
//
// Syntax: WebDOMHTMLInputElement.getType() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLInputElement::getType(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetType();

    retval->setNull();

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetType().wc_str());
}

// (METHOD) WebDOMHTMLInputElement.setType
// Description:
//
// Syntax: WebDOMHTMLInputElement.setType(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLInputElement::setType(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetType(const wxString& value);

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetType(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLInputElement.getUseMap
// Description:
//
// Syntax: WebDOMHTMLInputElement.getUseMap() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLInputElement::getUseMap(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetUseMap();

    retval->setNull();

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetUseMap().wc_str());
}

// (METHOD) WebDOMHTMLInputElement.setUseMap
// Description:
//
// Syntax: WebDOMHTMLInputElement.setUseMap(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLInputElement::setUseMap(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetUseMap(const wxString& value);

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetUseMap(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLInputElement.blur
// Description:
//
// Syntax: WebDOMHTMLInputElement.blur()
//
// Remarks:

void WebDOMHTMLInputElement::blur(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void Blur();

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    e.Blur();
}

// (METHOD) WebDOMHTMLInputElement.focus
// Description:
//
// Syntax: WebDOMHTMLInputElement.focus()
//
// Remarks:

void WebDOMHTMLInputElement::focus(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void Focus();

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    e.Focus();
}

// (METHOD) WebDOMHTMLInputElement.select
// Description:
//
// Syntax: WebDOMHTMLInputElement.select()
//
// Remarks:

void WebDOMHTMLInputElement::select(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void Select();

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    e.Select();
}

// (METHOD) WebDOMHTMLInputElement.click
// Description:
//
// Syntax: WebDOMHTMLInputElement.click()
//
// Remarks:

void WebDOMHTMLInputElement::click(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void Click();

    wxDOMHTMLInputElement e = m_node;
    if (!e.IsOk())
        return;

    e.Click();
}

// (CLASS) WebDOMHTMLLinkElement
// Category: DOM
// Derives: WebDOMHTMLElement
// Description: A class that represents an HTML link element.
// Remarks: The WebDOMHTMLLinkElement class represents an HTML link element.

WebDOMHTMLLinkElement::WebDOMHTMLLinkElement()
{
}

WebDOMHTMLLinkElement::~WebDOMHTMLLinkElement()
{
}

// (CONSTRUCTOR) WebDOMHTMLLinkElement.constructor
//
// Description: Creates a new WebDOMHTMLLinkElement object.
//
// Syntax: WebDOMHTMLLinkElement()
//
// Remarks: Creates a new WebDOMHTMLLinkElement object.

void WebDOMHTMLLinkElement::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

// (METHOD) WebDOMHTMLLinkElement.getDisabled
// Description:
//
// Syntax: WebDOMHTMLLinkElement.getDisabled() : Boolean
//
// Remarks:
//
// Returns:

void WebDOMHTMLLinkElement::getDisabled(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // bool GetDisabled();

    retval->setNull();

    wxDOMHTMLLinkElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setBoolean(e.GetDisabled());
}

// (METHOD) WebDOMHTMLLinkElement.setDisabled
// Description:
//
// Syntax: WebDOMHTMLLinkElement.setDisabled(value : Boolean)
//
// Remarks:
// Param(value):

void WebDOMHTMLLinkElement::setDisabled(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetDisabled(bool value);

    wxDOMHTMLLinkElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetDisabled(env->getParam(0)->getBoolean());
}

// (METHOD) WebDOMHTMLLinkElement.getCharset
// Description:
//
// Syntax: WebDOMHTMLLinkElement.getCharset() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLLinkElement::getCharset(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetCharset();

    retval->setNull();

    wxDOMHTMLLinkElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetCharset().wc_str());
}

// (METHOD) WebDOMHTMLLinkElement.setCharset
// Description:
//
// Syntax: WebDOMHTMLLinkElement.setCharset(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLLinkElement::setCharset(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetCharset(const wxString& value);

    wxDOMHTMLLinkElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetCharset(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLLinkElement.getHref
// Description:
//
// Syntax: WebDOMHTMLLinkElement.getHref() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLLinkElement::getHref(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetHref();

    retval->setNull();

    wxDOMHTMLLinkElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetHref().wc_str());
}

// (METHOD) WebDOMHTMLLinkElement.setHref
// Description:
//
// Syntax: WebDOMHTMLLinkElement.setHref(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLLinkElement::setHref(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetHref(const wxString& value);

    wxDOMHTMLLinkElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetHref(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLLinkElement.getHreflang
// Description:
//
// Syntax: WebDOMHTMLLinkElement.getHreflang() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLLinkElement::getHreflang(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetHreflang();

    retval->setNull();

    wxDOMHTMLLinkElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetHreflang().wc_str());
}

// (METHOD) WebDOMHTMLLinkElement.setHreflang
// Description:
//
// Syntax: WebDOMHTMLLinkElement.setHreflang(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLLinkElement::setHreflang(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetHreflang(const wxString& value);

    wxDOMHTMLLinkElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetHreflang(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLLinkElement.getMedia
// Description:
//
// Syntax: WebDOMHTMLLinkElement.getMedia() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLLinkElement::getMedia(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetMedia();

    retval->setNull();

    wxDOMHTMLLinkElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetMedia().wc_str());
}

// (METHOD) WebDOMHTMLLinkElement.setMedia
// Description:
//
// Syntax: WebDOMHTMLLinkElement.setMedia(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLLinkElement::setMedia(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetMedia(const wxString& value);

    wxDOMHTMLLinkElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetMedia(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLLinkElement.getRel
// Description:
//
// Syntax: WebDOMHTMLLinkElement.getRel() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLLinkElement::getRel(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetRel();

    retval->setNull();

    wxDOMHTMLLinkElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetRel().wc_str());
}

// (METHOD) WebDOMHTMLLinkElement.setRel
// Description:
//
// Syntax: WebDOMHTMLLinkElement.setRel(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLLinkElement::setRel(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetRel(const wxString& value);

    wxDOMHTMLLinkElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetRel(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLLinkElement.getRev
// Description:
//
// Syntax: WebDOMHTMLLinkElement.getRev() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLLinkElement::getRev(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetRev();

    retval->setNull();

    wxDOMHTMLLinkElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetRev().wc_str());
}

// (METHOD) WebDOMHTMLLinkElement.setRev
// Description:
//
// Syntax: WebDOMHTMLLinkElement.setRev(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLLinkElement::setRev(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetRev(const wxString& value);

    wxDOMHTMLLinkElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetRev(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLLinkElement.getTarget
// Description:
//
// Syntax: WebDOMHTMLLinkElement.getTarget() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLLinkElement::getTarget(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetTarget();

    retval->setNull();

    wxDOMHTMLLinkElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetTarget().wc_str());
}

// (METHOD) WebDOMHTMLLinkElement.setTarget
// Description:
//
// Syntax: WebDOMHTMLLinkElement.setTarget(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLLinkElement::setTarget(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetTarget(const wxString& value);

    wxDOMHTMLLinkElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetTarget(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLLinkElement.getType
// Description:
//
// Syntax: WebDOMHTMLLinkElement.getType() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLLinkElement::getType(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetType();

    retval->setNull();

    wxDOMHTMLLinkElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetType().wc_str());
}

// (METHOD) WebDOMHTMLLinkElement.setType
// Description:
//
// Syntax: WebDOMHTMLLinkElement.setType(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLLinkElement::setType(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetType(const wxString& value);

    wxDOMHTMLLinkElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetType(env->getParam(0)->getString());
}

// (CLASS) WebDOMHTMLOptionElement
// Category: DOM
// Derives: WebDOMHTMLElement
// Description: A class that represents an HTML option element.
// Remarks: The WebDOMHTMLOptionElement class represents an HTML option element.

WebDOMHTMLOptionElement::WebDOMHTMLOptionElement()
{
}

WebDOMHTMLOptionElement::~WebDOMHTMLOptionElement()
{
}

// (CONSTRUCTOR) WebDOMHTMLOptionElement.constructor
//
// Description: Creates a new WebDOMHTMLOptionElement object.
//
// Syntax: WebDOMHTMLOptionElement()
//
// Remarks: Creates a new WebDOMHTMLOptionElement object.

void WebDOMHTMLOptionElement::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

// (METHOD) WebDOMHTMLOptionElement.getDefaultSelected
// Description:
//
// Syntax: WebDOMHTMLOptionElement.getDefaultSelected() : Boolean
//
// Remarks:
//
// Returns:

void WebDOMHTMLOptionElement::getDefaultSelected(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // bool GetDefaultSelected();

    retval->setNull();

    wxDOMHTMLOptionElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setBoolean(e.GetDefaultSelected());
}

// (METHOD) WebDOMHTMLOptionElement.setDefaultSelected
// Description:
//
// Syntax: WebDOMHTMLOptionElement.setDefaultSelected(value : Boolean)
//
// Remarks:
// Param(value):

void WebDOMHTMLOptionElement::setDefaultSelected(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetDefaultSelected(bool value);

    wxDOMHTMLOptionElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetDefaultSelected(env->getParam(0)->getBoolean());
}

// (METHOD) WebDOMHTMLOptionElement.getText
// Description:
//
// Syntax: WebDOMHTMLOptionElement.getText() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLOptionElement::getText(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetText();

    retval->setNull();

    wxDOMHTMLOptionElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetText().wc_str());
}

// (METHOD) WebDOMHTMLOptionElement.getIndex
// Description:
//
// Syntax: WebDOMHTMLOptionElement.getIndex() : Integer
//
// Remarks:
//
// Returns:

void WebDOMHTMLOptionElement::getIndex(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // int GetIndex();

    retval->setNull();

    wxDOMHTMLOptionElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setInteger(e.GetIndex());
}

// (METHOD) WebDOMHTMLOptionElement.getDisabled
// Description:
//
// Syntax: WebDOMHTMLOptionElement.getDisabled() : Boolean
//
// Remarks:
//
// Returns:

void WebDOMHTMLOptionElement::getDisabled(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // bool GetDisabled();

    retval->setNull();

    wxDOMHTMLOptionElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setBoolean(e.GetDisabled());
}

// (METHOD) WebDOMHTMLOptionElement.setDisabled
// Description:
//
// Syntax: WebDOMHTMLOptionElement.setDisabled(value : Boolean)
//
// Remarks:
// Param(value):

void WebDOMHTMLOptionElement::setDisabled(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetDisabled(bool value);

    wxDOMHTMLOptionElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetDisabled(env->getParam(0)->getBoolean());
}

// (METHOD) WebDOMHTMLOptionElement.getLabel
// Description:
//
// Syntax: WebDOMHTMLOptionElement.getLabel() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLOptionElement::getLabel(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetLabel();

    retval->setNull();

    wxDOMHTMLOptionElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetLabel().wc_str());
}

// (METHOD) WebDOMHTMLOptionElement.setLabel
// Description:
//
// Syntax: WebDOMHTMLOptionElement.setLabel(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLOptionElement::setLabel(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetLabel(const wxString& value);

    wxDOMHTMLOptionElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetLabel(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLOptionElement.getSelected
// Description:
//
// Syntax: WebDOMHTMLOptionElement.getSelected() : Boolean
//
// Remarks:
//
// Returns:

void WebDOMHTMLOptionElement::getSelected(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // bool GetSelected();

    retval->setNull();

    wxDOMHTMLOptionElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setBoolean(e.GetSelected());
}

// (METHOD) WebDOMHTMLOptionElement.setSelected
// Description:
//
// Syntax: WebDOMHTMLOptionElement.setSelected(value : Boolean)
//
// Remarks:
// Param(value):

void WebDOMHTMLOptionElement::setSelected(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetSelected(bool value);

    wxDOMHTMLOptionElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetSelected(env->getParam(0)->getBoolean());
}

// (CLASS) WebDOMHTMLSelectElement
// Category: DOM
// Derives: WebDOMHTMLElement
// Description: A class that represents an HTML select element.
// Remarks: The WebDOMHTMLSelectElement class represents an HTML select element.

WebDOMHTMLSelectElement::WebDOMHTMLSelectElement()
{
}

WebDOMHTMLSelectElement::~WebDOMHTMLSelectElement()
{
}

// (CONSTRUCTOR) WebDOMHTMLSelectElement.constructor
//
// Description: Creates a new WebDOMHTMLSelectElement object.
//
// Syntax: WebDOMHTMLSelectElement()
//
// Remarks: Creates a new WebDOMHTMLSelectElement object.

void WebDOMHTMLSelectElement::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{ 
}

// (METHOD) WebDOMHTMLSelectElement.getType
// Description:
//
// Syntax: WebDOMHTMLSelectElement.getType() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLSelectElement::getType(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetType();

    retval->setNull();

    wxDOMHTMLSelectElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetType().wc_str());
}

// (METHOD) WebDOMHTMLSelectElement.getSelectedIndex
// Description:
//
// Syntax: WebDOMHTMLSelectElement.getSelectedIndex() : Integer
//
// Remarks:
//
// Returns:

void WebDOMHTMLSelectElement::getSelectedIndex(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // int GetSelectedIndex();

    retval->setNull();

    wxDOMHTMLSelectElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setInteger(e.GetSelectedIndex());
}

// (METHOD) WebDOMHTMLSelectElement.setSelectedIndex
// Description:
//
// Syntax: WebDOMHTMLSelectElement.setSelectedIndex(value : Integer)
//
// Remarks:
// Param(value):

void WebDOMHTMLSelectElement::setSelectedIndex(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetSelectedIndex(int index);

    wxDOMHTMLSelectElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetSelectedIndex(env->getParam(0)->getInteger());
}

// (METHOD) WebDOMHTMLSelectElement.getLength
// Description:
//
// Syntax: WebDOMHTMLSelectElement.getLength() : Integer
//
// Remarks:
//
// Returns:

void WebDOMHTMLSelectElement::getLength(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // int GetLength();

    retval->setNull();

    wxDOMHTMLSelectElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setInteger(e.GetLength());
}

// (METHOD) WebDOMHTMLSelectElement.setLength
// Description:
//
// Syntax: WebDOMHTMLSelectElement.setLength(value : Integer)
//
// Remarks:
// Param(value):

void WebDOMHTMLSelectElement::setLength(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetLength(int length);

    wxDOMHTMLSelectElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetLength(env->getParam(0)->getInteger());
}

// (METHOD) WebDOMHTMLSelectElement.getDisabled
// Description:
//
// Syntax: WebDOMHTMLSelectElement.getDisabled() : Boolean
//
// Remarks:
//
// Returns:

void WebDOMHTMLSelectElement::getDisabled(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // bool GetDisabled();

    retval->setNull();

    wxDOMHTMLSelectElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setBoolean(e.GetDisabled());
}

// (METHOD) WebDOMHTMLSelectElement.setDisabled
// Description:
//
// Syntax: WebDOMHTMLSelectElement.setDisabled(value : Boolean)
//
// Remarks:
// Param(value):

void WebDOMHTMLSelectElement::setDisabled(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetDisabled(bool value);

    wxDOMHTMLSelectElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetDisabled(env->getParam(0)->getBoolean());
}

// (METHOD) WebDOMHTMLSelectElement.getMultiple
// Description:
//
// Syntax: WebDOMHTMLSelectElement.getMultiple() : Boolean
//
// Remarks:
//
// Returns:

void WebDOMHTMLSelectElement::getMultiple(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // bool GetMultiple();

    retval->setNull();

    wxDOMHTMLSelectElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setBoolean(e.GetMultiple());
}

// (METHOD) WebDOMHTMLSelectElement.setMultiple
// Description:
//
// Syntax: WebDOMHTMLSelectElement.setMultiple(value : Boolean)
//
// Remarks:
// Param(value):

void WebDOMHTMLSelectElement::setMultiple(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetMultiple(bool value);

    wxDOMHTMLSelectElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetMultiple(env->getParam(0)->getBoolean());
}

// (METHOD) WebDOMHTMLSelectElement.getName
// Description:
//
// Syntax: WebDOMHTMLSelectElement.getName() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLSelectElement::getName(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetName();

    retval->setNull();

    wxDOMHTMLSelectElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetName().wc_str());
}

// (METHOD) WebDOMHTMLSelectElement.setName
// Description:
//
// Syntax: WebDOMHTMLSelectElement.setName(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLSelectElement::setName(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetName(const wxString& value);

    wxDOMHTMLSelectElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetName(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLSelectElement.getSize
// Description:
//
// Syntax: WebDOMHTMLSelectElement.getSize() : Integer
//
// Remarks:
//
// Returns:

void WebDOMHTMLSelectElement::getSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // int GetSize();

    retval->setNull();

    wxDOMHTMLSelectElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setInteger(e.GetSize());
}

// (METHOD) WebDOMHTMLSelectElement.setSize
// Description:
//
// Syntax: WebDOMHTMLSelectElement.setSize(value : Integer)
//
// Remarks:
// Param(value):

void WebDOMHTMLSelectElement::setSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetSize(int value);

    wxDOMHTMLSelectElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetSize(env->getParam(0)->getInteger());
}


// (METHOD) WebDOMHTMLSelectElement.add
// Description:
//
// Syntax: WebDOMHTMLSelectElement.add(element : WebDOMHTMLElement,
//                                     before : WebDOMHTMLElement)
//
// Remarks:
// Param(element):
// Param(before):

void WebDOMHTMLSelectElement::add(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void Add(const wxDOMHTMLElement& element, const wxDOMHTMLElement& before);

    retval->setBoolean(false);
    
    if (env->getParamCount() < 1)
        return;
        
    if (!env->getParam(0)->isObject())
        return;
        
    if (env->getParamCount() >= 2 && !env->getParam(1)->isObject())
        return;
        
    kscript::ValueObject* p0 = env->getParam(0)->getObject();
    kscript::ValueObject* p1 = NULL;
    
    if (env->getParamCount() >= 2)
        p1 = env->getParam(1)->getObject();
    
    if (!p0->isKindOf(L"WebDOMHTMLElement"))
        return;
    if (p1 && !p1->isKindOf(L"WebDOMHTMLElement"))
        return;

    wxDOMHTMLSelectElement node = m_node;
    if (node.IsOk())
    {
        WebDOMHTMLElement* w0 = (WebDOMHTMLElement*)p0;
        WebDOMHTMLElement* w1 = (WebDOMHTMLElement*)p1;

        wxDOMHTMLElement e0 = w0->m_node;
        wxDOMHTMLElement e1;
        if (w1)
            e1 = w1->m_node;
                   
        node.Add(e0, e1);
        retval->setBoolean(true);
    }
     else
    {
    }
}

// (METHOD) WebDOMHTMLSelectElement.remove
// Description:
//
// Syntax: WebDOMHTMLSelectElement.remove(value : Integer)
//
// Remarks:
// Param(value):

void WebDOMHTMLSelectElement::remove(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void Remove(int index);

    retval->setBoolean(false);

    if (env->getParamCount() < 1)
        return;
        
    wxDOMHTMLSelectElement node = m_node;
    if (node.IsOk())
    {
        node.Remove(env->getParam(0)->getInteger());
        retval->setBoolean(true);
    }
}

// (METHOD) WebDOMHTMLSelectElement.blur
// Description:
//
// Syntax: WebDOMHTMLSelectElement.blur()
//
// Remarks:

void WebDOMHTMLSelectElement::blur(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void Blur();

    wxDOMHTMLSelectElement e = m_node;
    if (!e.IsOk())
        return;

    e.Blur();
}

// (METHOD) WebDOMHTMLSelectElement.focus
// Description:
//
// Syntax: WebDOMHTMLSelectElement.focus()
//
// Remarks:

void WebDOMHTMLSelectElement::focus(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void Focus();

    wxDOMHTMLSelectElement e = m_node;
    if (!e.IsOk())
        return;

    e.Focus();
}

// (CLASS) WebDOMHTMLTextAreaElement
// Category: DOM
// Derives: WebDOMHTMLElement
// Description: A class that represents an HTML text area element.
// Remarks: The WebDOMHTMLTextAreaElement class represents an HTML text area element.

WebDOMHTMLTextAreaElement::WebDOMHTMLTextAreaElement()
{
}

WebDOMHTMLTextAreaElement::~WebDOMHTMLTextAreaElement()
{
}

// (CONSTRUCTOR) WebDOMHTMLTextAreaElement.constructor
//
// Description: Creates a new WebDOMHTMLTextAreaElement object.
//
// Syntax: WebDOMHTMLTextAreaElement()
//
// Remarks: Creates a new WebDOMHTMLTextAreaElement object.

void WebDOMHTMLTextAreaElement::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

// (METHOD) WebDOMHTMLTextAreaElement.getDefaultValue
// Description:
//
// Syntax: WebDOMHTMLTextAreaElement.getDefaultValue() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLTextAreaElement::getDefaultValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetDefaultValue();

    retval->setNull();

    wxDOMHTMLTextAreaElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetDefaultValue().wc_str());
}

// (METHOD) WebDOMHTMLTextAreaElement.setDefaultValue
// Description:
//
// Syntax: WebDOMHTMLTextAreaElement.setDefaultValue(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLTextAreaElement::setDefaultValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetDefaultValue(const wxString& value);

    wxDOMHTMLTextAreaElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetDefaultValue(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLTextAreaElement.getCols
// Description:
//
// Syntax: WebDOMHTMLTextAreaElement.getCols() : Integer
//
// Remarks:
//
// Returns:

void WebDOMHTMLTextAreaElement::getCols(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // int GetCols();

    retval->setNull();

    wxDOMHTMLTextAreaElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setInteger(e.GetCols());
}

// (METHOD) WebDOMHTMLTextAreaElement.setCols
// Description:
//
// Syntax: WebDOMHTMLTextAreaElement.setCols(value : Integer)
//
// Remarks:
// Param(value):

void WebDOMHTMLTextAreaElement::setCols(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetCols(int value);

    wxDOMHTMLTextAreaElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetCols(env->getParam(0)->getInteger());
}

// (METHOD) WebDOMHTMLTextAreaElement.getDisabled
// Description:
//
// Syntax: WebDOMHTMLTextAreaElement.getDisabled() : Boolean
//
// Remarks:
//
// Returns:

void WebDOMHTMLTextAreaElement::getDisabled(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // bool GetDisabled();

    retval->setNull();

    wxDOMHTMLTextAreaElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setBoolean(e.GetDisabled());
}

// (METHOD) WebDOMHTMLTextAreaElement.setDisabled
// Description:
//
// Syntax: WebDOMHTMLTextAreaElement.setDisabled(value : Boolean)
//
// Remarks:
// Param(value):

void WebDOMHTMLTextAreaElement::setDisabled(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetDisabled(bool value);

    wxDOMHTMLTextAreaElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetDisabled(env->getParam(0)->getBoolean());
}

// (METHOD) WebDOMHTMLTextAreaElement.getName
// Description:
//
// Syntax: WebDOMHTMLTextAreaElement.getName() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLTextAreaElement::getName(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetName();

    retval->setNull();

    wxDOMHTMLTextAreaElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetName().wc_str());
}

// (METHOD) WebDOMHTMLTextAreaElement.setName
// Description:
//
// Syntax: WebDOMHTMLTextAreaElement.setName(value : String)
//
// Remarks:
// Param(value):

void WebDOMHTMLTextAreaElement::setName(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetName(const wxString& value);

    wxDOMHTMLTextAreaElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetName(env->getParam(0)->getString());
}

// (METHOD) WebDOMHTMLTextAreaElement.getReadOnly
// Description:
//
// Syntax: WebDOMHTMLTextAreaElement.getReadOnly() : Boolean
//
// Remarks:
//
// Returns:

void WebDOMHTMLTextAreaElement::getReadOnly(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // bool GetReadOnly();

    retval->setNull();

    wxDOMHTMLTextAreaElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setBoolean(e.GetReadOnly());
}

// (METHOD) WebDOMHTMLTextAreaElement.setReadOnly
// Description:
//
// Syntax: WebDOMHTMLTextAreaElement.setReadOnly(value : Boolean)
//
// Remarks:
// Param(value):

void WebDOMHTMLTextAreaElement::setReadOnly(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetReadOnly(bool value);

    wxDOMHTMLTextAreaElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetReadOnly(env->getParam(0)->getBoolean());
}

// (METHOD) WebDOMHTMLTextAreaElement.getRows
// Description:
//
// Syntax: WebDOMHTMLTextAreaElement.getRows() : Integer
//
// Remarks:
//
// Returns:

void WebDOMHTMLTextAreaElement::getRows(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // int GetRows();

    retval->setNull();

    wxDOMHTMLTextAreaElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setInteger(e.GetRows());
}

// (METHOD) WebDOMHTMLTextAreaElement.setRows
// Description:
//
// Syntax: WebDOMHTMLTextAreaElement.setRows(value : Integer)
//
// Remarks:
// Param(value):

void WebDOMHTMLTextAreaElement::setRows(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void SetRows(int value);

    wxDOMHTMLTextAreaElement e = m_node;
    if (!e.IsOk())
        return;

    if (env->getParamCount() < 1)
        return;

    e.SetRows(env->getParam(0)->getInteger());
}

// (METHOD) WebDOMHTMLTextAreaElement.getType
// Description:
//
// Syntax: WebDOMHTMLTextAreaElement.getType() : String
//
// Remarks:
//
// Returns:

void WebDOMHTMLTextAreaElement::getType(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // wxString GetType();

    retval->setNull();

    wxDOMHTMLTextAreaElement e = m_node;
    if (!e.IsOk())
        return;

    retval->setString(e.GetType().wc_str());
}

// (METHOD) WebDOMHTMLTextAreaElement.blur
// Description:
//
// Syntax: WebDOMHTMLTextAreaElement.blur()
//
// Remarks:

void WebDOMHTMLTextAreaElement::blur(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void Blur();
    
    wxDOMHTMLTextAreaElement e = m_node;
    if (!e.IsOk())
        return;

    e.Blur();
}

// (METHOD) WebDOMHTMLTextAreaElement.focus
// Description:
//
// Syntax: WebDOMHTMLTextAreaElement.focus()
//
// Remarks:

void WebDOMHTMLTextAreaElement::focus(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void Focus();

    wxDOMHTMLTextAreaElement e = m_node;
    if (!e.IsOk())
        return;

    e.Focus();
}

// (METHOD) WebDOMHTMLTextAreaElement.select
// Description:
//
// Syntax: WebDOMHTMLTextAreaElement.select()
//
// Remarks:

void WebDOMHTMLTextAreaElement::select(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: fill out documentation
    // void Select();

    wxDOMHTMLTextAreaElement e = m_node;
    if (!e.IsOk())
        return;

    e.Select();
}

