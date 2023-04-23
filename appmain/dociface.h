/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2009-01-01
 *
 */


#ifndef H_APP_DOCIFACE_H
#define H_APP_DOCIFACE_H


// forward declarations
namespace kscript
{
    class Value;
}


xcm_interface IFindTarget : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IFindTarget")
    
public:

    virtual bool findNextMatch(
                            const wxString& expr,
                            bool forward,
                            bool match_case,
                            bool whole) = 0;
    virtual bool findReplaceWith(
                            const wxString& find_val,
                            const wxString& replace_val,
                            bool forward,
                            bool match_case,
                            bool whole) = 0;
    virtual bool findReplaceAll(
                            const wxString& find_val,
                            const wxString& replace_val,
                            bool match_case,
                            bool whole) = 0;
    virtual bool findIsReplaceAllowed() = 0;
};

XCM_DECLARE_SMARTPTR(IFindTarget);




xcm_interface IDocumentScriptBinding : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IDocumentScriptBinding")
    
public:

    virtual bool getScriptMember(const std::wstring& member,
                                 kscript::Value* retval) = 0;
};

XCM_DECLARE_SMARTPTR(IDocumentScriptBinding);





#endif

