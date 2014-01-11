/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2001-08-21
 *
 */


#ifndef __KL_XML_H
#define __KL_XML_H


#include <vector>
#include <string>


namespace kl
{


class xmlproperty
{
public:

    bool isEmpty()
    {
        return name.empty();
    }
    
public:
    std::wstring name;
    std::wstring value;
};

class xmlnode;



class xmlnode
{
public:

    enum
    {
        formattingTabs         = 0x0001,
        formattingSpaces       = 0x0002,
        formattingLf           = 0x0004,
        formattingCrLf         = 0x0008,
        formattingOmitPrologue = 0x8000
    };

    enum
    {
        parseRelaxed = 0x0001
    };
    
    enum
    {
        filemodeShareRead = 1,
        filemodeExclusive = 2,
        filemodeExclusiveWait = 3
    };

public:

    xmlnode();
    ~xmlnode();
    
    bool parse(const wchar_t* xml_text, int parse_flags = 0);
    bool parse(const char* xml_text, int parse_flags = 0);
    bool parse(const std::wstring& xml_text, int parse_flags = 0);
    bool parse(const std::string& xml_text, int parse_flags = 0);

    bool load(const std::wstring& filename, int parse_flags = 0, int file_mode = filemodeShareRead);
    bool save(const std::wstring& filename, int formatting_flags = 0, int file_mode = filemodeExclusive);

    xmlnode& copy(xmlnode& node, bool deep = true);
    xmlnode& clone(bool deep = true);
    void clear();

    xmlnode& insertChild(size_t idx);
    xmlnode& addChild();
    xmlnode& addChild(const std::wstring& tag_name);
    xmlnode& addChild(const std::wstring& tag_name, const std::wstring& contents);
    xmlnode& addChild(const std::wstring& tag_name, int contents);
    
    void setNodeName(const std::wstring& tag_name);
    void setNodeValue(const std::wstring& contents);
    void setNodeValue(int contents);

    const std::wstring& getNodeName() const;
    const std::wstring& getNodeValue() const;

    std::wstring getXML(int format_flags = formattingSpaces | formattingCrLf) const;

    size_t getChildCount() const;
    bool hasChildNodes() const;
    xmlnode& getChild(const std::wstring& tag_name);
    xmlnode& getChild(size_t idx);
    
    void deleteAllChildren();
    bool removeChild(size_t idx);
    bool removeChild(const kl::xmlnode& node);
    
    bool isEmpty() const;
    int getChildIdx(const std::wstring& tag_name);
    int lookupChild(const std::wstring& tag_name,
                    const std::wstring& prop_name,
                    const std::wstring& prop_value);
                    
    xmlproperty& appendProperty();
    xmlproperty& appendProperty(const std::wstring& name,
                                const std::wstring& value);
    size_t getPropertyCount() const;
    xmlproperty& getProperty(size_t idx);
    xmlproperty& getProperty(const std::wstring& name);

    int getPropertyIdx(const std::wstring& name) const;
    const std::wstring& getPropertyValue(size_t idx) const;

private:

    static int getLengthAfterTokenization(const wchar_t* src);
    static void tokenizeAppend(std::wstring& dest, const std::wstring& src);
    static void detokenize(std::wstring& dest, const wchar_t* src, int src_len);
    bool parseProperty(const wchar_t* p, const wchar_t** endpos);
    bool internalParse(const wchar_t* xml_text, const wchar_t** next_start, int parse_flags);
    void internalPrint(std::wstring& output,
                       int format_flags,
                       int indent,
                       const wchar_t* cr,
                       wchar_t indent_char) const;
    int getLengthEstimate(int indent) const;
    void internalGetXML(std::wstring& result,
                        int format_flags = formattingSpaces | formattingCrLf) const;
    void internalClone(xmlnode* original, xmlnode** copy, bool deep = true);

private:

    std::vector<xmlnode*> m_nodes;
    std::vector<xmlproperty*> m_properties;
    std::wstring m_tag_name;
    std::wstring m_contents;
};



};



#endif

