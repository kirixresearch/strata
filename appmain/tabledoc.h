/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-03-06
 *
 */


#ifndef __APP_TABLEDOC_H
#define __APP_TABLEDOC_H



// -- Frame events fired by this document: --------------------------------------
//
//
// "tabledoc.viewModified"        - fired when the active view is modified
//                                  param 1: 0=columns changed, 1=column resize
//
// "tabledoc.viewChanged"         - fired when the current view is set
//
//
// "tabledoc.structureModified"   - fired when the table's structure has been
//                                  modified. Example: calcfield creation
//                                  or deletion
//
// "tabledoc.markCreated"         - fired when a new mark has been created by
//                                  the document
//
// "tabledoc.enabledStateChanged" - fired when the tabledoc was enabled or disabled
//
// ------------------------------------------------------------------------------




// -- forward declarations --
xcm_interface ITableDocObject;
xcm_interface ITableDocMark;
xcm_interface ITableDocViewCol;
xcm_interface ITableDocView;
xcm_interface ITableDocModel;
xcm_interface ITableDoc;

XCM_DECLARE_SMARTPTR(ITableDocObject)
XCM_DECLARE_SMARTPTR(ITableDocMark)
XCM_DECLARE_SMARTPTR(ITableDocViewCol)
XCM_DECLARE_SMARTPTR(ITableDocView)
XCM_DECLARE_SMARTPTR(ITableDocModel)
XCM_DECLARE_SMARTPTR(ITableDoc)
XCM_DECLARE_SMARTPTR2(xcm::IVector<ITableDocObjectPtr>, ITableDocObjectEnumPtr)
XCM_DECLARE_SMARTPTR2(xcm::IVector<ITableDocMarkPtr>, ITableDocMarkEnumPtr)
XCM_DECLARE_SMARTPTR2(xcm::IVector<ITableDocViewPtr>, ITableDocViewEnumPtr)


enum
{
    tabledocAlignDefault = 0,
    tabledocAlignLeft = 1,
    tabledocAlignCenter = 2,
    tabledocAlignRight = 3
};

enum
{
    tabledocWrapDefault = 0,
    tabledocWrapOn = 1,
    tabledocWrapOff = 2
};

enum
{
    tabledocRelationshipSyncNone = 0,
    tabledocRelationshipSyncFilter = 1,
    tabledocRelationshipSyncSeek = 2
};



xcm_interface ITableDocObject : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.ITableDocStorable")

public:

    virtual wxString getObjectId() = 0;
    virtual void setObjectId(const wxString& id) = 0;

    virtual ITableDocObjectPtr clone() = 0;

    virtual bool getDirty() = 0;
    virtual void setDirty(bool dirty) = 0;

    virtual bool readFromNode(kl::JsonNode& node) = 0;
    virtual bool writeToNode(kl::JsonNode& node) = 0;

    virtual bool readFromNode(tango::INodeValuePtr node) = 0;
    virtual bool writeToNode(tango::INodeValuePtr node) = 0;
};


xcm_interface ITableDocMark : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.ITableDocMark")

public:

    virtual void setDescription(const wxString& description) = 0;
    virtual wxString getDescription() = 0;

    virtual void setExpression(const wxString& expr) = 0;
    virtual wxString getExpression() = 0;

    virtual void setMarkActive(bool new_val) = 0;
    virtual bool getMarkActive() = 0;

    virtual void setForegroundColor(const wxColour& color) = 0;
    virtual wxColour getForegroundColor() = 0;

    virtual void setBackgroundColor(const wxColour& color) = 0;
    virtual wxColor getBackgroundColor() = 0;
};




xcm_interface ITableDocViewCol : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.ITableDocViewCol")

public:

    virtual void setName(const wxString& new_value) = 0;
    virtual wxString getName() = 0;

    virtual void setSize(int new_value) = 0;
    virtual int getSize() = 0;

    virtual void setForegroundColor(const wxColor& new_value) = 0;
    virtual wxColor getForegroundColor() = 0;

    virtual void setBackgroundColor(const wxColor& new_value) = 0;
    virtual wxColor getBackgroundColor() = 0;

    virtual void setAlignment(int new_value) = 0;
    virtual int getAlignment() = 0;
    
    virtual void setTextWrap(int new_value) = 0;
    virtual int getTextWrap() = 0;
};


xcm_interface ITableDocView : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.ITableDocView")

public:

    virtual void setDescription(const wxString& new_val) = 0;
    virtual wxString getDescription() = 0;

    virtual void setRowSize(int new_value) = 0;
    virtual int getRowSize() = 0;

    virtual int getColumnCount() = 0;
    virtual ITableDocViewColPtr getColumn(unsigned int idx) = 0;
    virtual int getColumnIdx(const wxString& col_name) = 0;
    virtual ITableDocViewColPtr createColumn(int pos) = 0;
    virtual void deleteColumn(unsigned int idx) = 0;
    virtual void deleteAllColumns() = 0;
    virtual void moveColumn(unsigned int old_idx, unsigned int new_idx) = 0;
};



xcm_interface ITableDocModel : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.ITableDocModel")

public:

    virtual wxString getId() = 0;

    virtual bool writeObject(ITableDocObjectPtr obj) = 0;
    virtual bool writeMultipleObjects(ITableDocObjectEnumPtr obj) = 0;
    virtual bool deleteObject(ITableDocObjectPtr obj) = 0;

    virtual ITableDocMarkPtr createMarkObject() = 0;
    virtual ITableDocViewPtr createViewObject() = 0;

    virtual ITableDocMarkEnumPtr getMarkEnum() = 0;
    virtual ITableDocViewEnumPtr getViewEnum() = 0;
    
    virtual wxColor getNextMarkColor() = 0;
};



xcm_interface ITableDoc : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.ITableDoc")

public:

    virtual bool open(tango::IDatabasePtr db,
                      const wxString& table,
                      tango::ISetPtr optional_set = xcm::null,
                      tango::IIteratorPtr optional_iterator = xcm::null) = 0;

    virtual bool open(tango::ISetPtr set,
                      tango::IIteratorPtr iter) = 0;

    virtual bool setBrowseSet(tango::ISetPtr set,
                              tango::IIteratorPtr iter) = 0;

    virtual void setCaption(const wxString& caption,
                            const wxString& caption_suffix) = 0;

    // url that generated this tabledoc (rss, csv, etc.)
    // this will override the value returned by IDocument::getDocumentLocation()
    virtual void setSourceUrl(const wxString& source_url) = 0;
    virtual void setSourceMimeType(const wxString& source_mimetype) = 0;

    virtual wxString getCaption() = 0;

    virtual void closeSet() = 0;
    virtual void setEnabled(bool new_val) = 0;
    virtual bool getEnabled() = 0;

    virtual void setTemporaryModel(bool new_val) = 0;

    virtual void setIterator(tango::IIteratorPtr iter, bool go_first = true) = 0;
    virtual kcl::Grid* getGrid() = 0;
    virtual tango::IIteratorPtr getIterator() = 0;
    virtual tango::ISetPtr getBaseSet() = 0;
    virtual tango::ISetPtr getBrowseSet() = 0;
    virtual ITableDocModelPtr getModel() = 0;

    virtual void connectAlterTableJob(jobs::IJobPtr job) = 0;

    virtual void onColumnNameChanged(const wxString& old_name,
                                     const wxString& new_name) = 0;

    virtual void setSortOrder(const wxString& new_value) = 0;
    virtual wxString getSortOrder() = 0;
    virtual wxString getFilter() = 0;
    virtual void setQuickFilter(const wxString& val) = 0;
    virtual void setFilter(const wxString& condition) = 0;
    virtual void removeFilter() = 0;
    virtual void copyRecords(const wxString& condition) = 0;
    virtual void deleteRecords(const wxString& condition) = 0;
    virtual void insertColumnSeparator(int insert_pos) = 0;
    virtual void insertColumn(int insert_pos, const wxString& col_name) = 0;
    virtual void hideColumn(int idx) = 0;
    virtual void setFont(const wxFont& font) = 0;
    virtual void reloadSettings(bool redraw) = 0;
    virtual void showReplacePanel(const wxString& def_condition, const wxString& def_field = wxT("")) = 0;
    virtual void showViewPanel() = 0;
    virtual void showIndexPanel() = 0;
    virtual void showCreateDynamicField() = 0;
    virtual void showEditDynamicField(const wxString& column_name) = 0;
    virtual void deleteAllRelations() = 0;
    virtual int getRelationshipSync() = 0;
    virtual void setRelationshipSync(int state) = 0;
    virtual void setIsChildSet(bool new_val) = 0;
    virtual void setRelationshipSyncMarkExpr(const wxString& expr) = 0;
    virtual bool getIsChildSet() = 0;

    virtual void updateChildWindows() = 0;
    virtual void updateStatusBar() = 0;

    virtual void setActiveView(ITableDocViewPtr view) = 0;
    virtual ITableDocViewPtr getActiveView() = 0;
    virtual void refreshActiveView(bool repaint = true) = 0;
    
    virtual bool isQuickFilterPending() = 0;    
};






// -- tabledoc external API --

class TableDocMgr
{
public:

    static bool newFile(const wxString& path);
    static ITableDocPtr createTableDoc();
    static ITableDocPtr getActiveTableDoc(int* site_id = NULL);

    static ITableDocModelPtr loadModel(const wxString& set_id);
    static bool deleteModel(const wxString& set_id);
    static void copyModel(tango::ISetPtr src_id,
                          tango::ISetPtr dest_id);
    static void clearModelRegistry();
    static void cleanupModelRegistry();
};




#endif


