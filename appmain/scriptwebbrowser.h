/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Aaron L. Williams; Benjamin I. Williams
 * Created:  2007-08-03
 *
 */


#ifndef __APP_SCRIPTWEBBROWSER_H
#define __APP_SCRIPTWEBBROWSER_H


class WebBrowser : public FormControl
{
    BEGIN_KSCRIPT_DERIVED_CLASS("WebBrowser", WebBrowser, FormControl)
        KSCRIPT_GUI_METHOD("constructor", WebBrowser::constructor)
        KSCRIPT_GUI_METHOD("navigate", WebBrowser::navigate)
        KSCRIPT_GUI_METHOD("showHypertext", WebBrowser::showHypertext)
        KSCRIPT_METHOD    ("waitUntilReady", WebBrowser::waitUntilReady)
        KSCRIPT_GUI_METHOD("getLocation", WebBrowser::getLocation)
        KSCRIPT_GUI_METHOD("goForward", WebBrowser::goForward)
        KSCRIPT_GUI_METHOD("goBack", WebBrowser::goBack)
        KSCRIPT_GUI_METHOD("reload", WebBrowser::reload)
        KSCRIPT_GUI_METHOD("stop", WebBrowser::stop)
        KSCRIPT_GUI_METHOD("canCutSelection", WebBrowser::canCutSelection)
        KSCRIPT_GUI_METHOD("canCopySelection", WebBrowser::canCopySelection)
        KSCRIPT_GUI_METHOD("canCopyLinkLocation", WebBrowser::canCopyLinkLocation)
        KSCRIPT_GUI_METHOD("canCopyImageLocation", WebBrowser::canCopyImageLocation)
        KSCRIPT_GUI_METHOD("canCopyImageContents", WebBrowser::canCopyImageContents)
        KSCRIPT_GUI_METHOD("canPaste", WebBrowser::canPaste)
        KSCRIPT_GUI_METHOD("cutSelection", WebBrowser::cutSelection)
        KSCRIPT_GUI_METHOD("copySelection", WebBrowser::copySelection)
        KSCRIPT_GUI_METHOD("copyLinkLocation", WebBrowser::copyLinkLocation)
        KSCRIPT_GUI_METHOD("copyImageLocation", WebBrowser::copyImageLocation)
        KSCRIPT_GUI_METHOD("copyImageContents", WebBrowser::copyImageContents)
        KSCRIPT_GUI_METHOD("paste", WebBrowser::paste)
        KSCRIPT_GUI_METHOD("selectAll", WebBrowser::selectAll)
        KSCRIPT_GUI_METHOD("selectNone", WebBrowser::selectNone)
        KSCRIPT_GUI_METHOD("getDOMDocument", WebBrowser::getDOMDocument)
    END_KSCRIPT_CLASS()

public:

    WebBrowser();
    ~WebBrowser();
    void realize();
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void navigate(kscript::ExprEnv* env, kscript::Value* retval);
    void showHypertext(kscript::ExprEnv* env, kscript::Value* retval);
    void waitUntilReady(kscript::ExprEnv* env, kscript::Value* retval);
    void getLocation(kscript::ExprEnv* env, kscript::Value* retval);
    void goForward(kscript::ExprEnv* env, kscript::Value* retval);
    void goBack(kscript::ExprEnv* env, kscript::Value* retval);
    void reload(kscript::ExprEnv* env, kscript::Value* retval);
    void stop(kscript::ExprEnv* env, kscript::Value* retval);
    void canCutSelection(kscript::ExprEnv* env, kscript::Value* retval);
    void canCopySelection(kscript::ExprEnv* env, kscript::Value* retval);
    void canCopyLinkLocation(kscript::ExprEnv* env, kscript::Value* retval);
    void canCopyImageLocation(kscript::ExprEnv* env, kscript::Value* retval);
    void canCopyImageContents(kscript::ExprEnv* env, kscript::Value* retval);
    void canPaste(kscript::ExprEnv* env, kscript::Value* retval);
    void cutSelection(kscript::ExprEnv* env, kscript::Value* retval);
    void copySelection(kscript::ExprEnv* env, kscript::Value* retval);
    void copyLinkLocation(kscript::ExprEnv* env, kscript::Value* retval);
    void copyImageLocation(kscript::ExprEnv* env, kscript::Value* retval);
    void copyImageContents(kscript::ExprEnv* env, kscript::Value* retval);
    void paste(kscript::ExprEnv* env, kscript::Value* retval);
    void selectAll(kscript::ExprEnv* env, kscript::Value* retval);
    void selectNone(kscript::ExprEnv* env, kscript::Value* retval);
    void getDOMDocument(kscript::ExprEnv* env, kscript::Value* retval);

    bool showHypertext(const wxString& hypertext);
    
private:

    void onEvent(wxEvent& evt);
    
private:

    wxWebControl* m_ctrl;
    wxString m_initial_nav;
    wxString m_initial_html;
    std::vector<wxString> m_tempfiles_to_delete;
};



#endif

