/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   Benjamin I. Williams
 * Created:  2003-05-06
 *
 */


#ifndef __KXINTER_DATADOC_H
#define __KXINTER_DATADOC_H


#include <map>
#include "datastream.h"
#include "datadef.h"


class DataViewBase;



struct RecordInfo
{
    xf_off_t offset;
    int len;
};



bool setActiveLayoutFile(const wxString& data_file, const wxString& layout_file);
wxString getActiveLayoutFile(const wxString& data_file);



class DataDoc : public wxWindow,
                public cfw::IDocument
{

XCM_CLASS_NAME_NOREFCOUNT("kxinter.DataDoc")
XCM_BEGIN_INTERFACE_MAP(DataDoc)
    XCM_INTERFACE_ENTRY(cfw::IDocument)
XCM_END_INTERFACE_MAP()

public:

    enum
    {
        viewTextRecord = 0,
        viewHex = 1
    };

public:

    DataDoc();
    ~DataDoc();

    bool open(const wxString& filename);
    void setActiveView(int view_type);
    int defineField(int record_len, int offset, int length, int type);

    bool saveFileInfo();
    bool loadFileInfo();


private:

    cfw::IDocumentSitePtr showDataFormatPanel();
    void updateScrollbars();

    // -- IDocument interface implementation --
    bool initDoc(cfw::IFramePtr frame, cfw::IDocumentSitePtr site, wxWindow* docsite_wnd, wxWindow* panesite_wnd);
    wxWindow* getDocumentWindow();
    void setDocumentFocus();
    bool onSiteClosing(bool force);

private:

    void onSize(wxSizeEvent& event);
    void onTextView(wxCommandEvent& event);
    void onHexView(wxCommandEvent& event);
    void onAscii(wxCommandEvent& event);
    void onEbcdic(wxCommandEvent& event);
    void onRectypeChanged(wxCommandEvent& event);
    void onUpdateUI(wxUpdateUIEvent& event);
    void onDataFormatEditor(wxCommandEvent& event);
    void onDefineField(wxCommandEvent& event);
    void onExecute(wxCommandEvent& event);
    void onSetActiveLayout(wxCommandEvent& event);
    void onDetachActiveLayout(wxCommandEvent& event);
    void onImportLayout(wxCommandEvent& event);
    void onExportLayout(wxCommandEvent& event);
    void onSetRecordLength(wxCommandEvent& event);
    void onSetSkipBytes(wxCommandEvent& event);
    void onJumpToPosition(wxCommandEvent& event);

private:
    cfw::IUIContextPtr getUserInterface(cfw::IFramePtr frame);
    
public:

    cfw::IDocumentSitePtr m_doc_site;
    wxBoxSizer* m_main_sizer;
    wxStatusBar* m_status_bar;
    kcl::RulerControl* m_top_ruler;
    wxFont m_font;
    IDataStreamPtr m_data;
    wxString m_filename;
    wxString m_dfd_filename;
    ds_off_t m_offset;
    DataViewBase* m_view;
    wxString m_caption;
    DataFormatDefinition m_dfd;
    int m_view_type;
    unsigned char* m_buf;
    bool m_ebc;

    std::map<int, RecordInfo> m_records;

    unsigned char getByte(int offset);
    int getRecord(int recno, unsigned char* buf, int max_read, RecordInfo* out_info);

    void setRecordLength(int new_val);
    int getRecordLength();
    void updateStatusBar();

    DECLARE_EVENT_TABLE()
};




#endif



