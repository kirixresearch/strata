/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   Benjamin I. Williams
 * Created:  2003-05-06
 *
 */


#include "kxinter.h"
#include "datadoc.h"
#include "textview.h"
#include "hexview.h"
#include "paneldataformat.h"
#include "jobconvert.h"
#include "copybookparse.h"
#include "appcontroller.h"


// -- minimum size = 65536 --
const int DATA_PAGE_SIZE = 65536;


enum
{
    ID_FirstDataDocCommand = cfw::FirstDocCommandId,

    ID_DataFormatEditor,
    ID_TextView,
    ID_HexView,
    ID_Ascii,
    ID_Ebcdic,
    ID_DefineField,
    ID_Execute,
    ID_SetActiveLayout,
    ID_DetachActiveLayout,
    ID_ImportLayout,
    ID_ExportLayout,
    ID_SetRecordLength,
    ID_SetSkipBytes,
    ID_JumpToPosition,
    ID_Fixed,
    ID_Variable,
    ID_Delimited,

    ID_LastDataDocCommand
};



BEGIN_EVENT_TABLE(DataDoc, wxWindow)
    EVT_SIZE(DataDoc::onSize)
    EVT_MENU(ID_TextView, DataDoc::onTextView)
    EVT_MENU(ID_HexView, DataDoc::onHexView)
    EVT_MENU(ID_Ascii, DataDoc::onAscii)
    EVT_MENU(ID_Ebcdic, DataDoc::onEbcdic)
    EVT_MENU(ID_DefineField, DataDoc::onDefineField)
    EVT_MENU(ID_DataFormatEditor, DataDoc::onDataFormatEditor)
    EVT_MENU(ID_Execute, DataDoc::onExecute)
    EVT_MENU(ID_SetActiveLayout, DataDoc::onSetActiveLayout)
    EVT_MENU(ID_DetachActiveLayout, DataDoc::onDetachActiveLayout)
    EVT_MENU(ID_ImportLayout, DataDoc::onImportLayout)
    EVT_MENU(ID_ExportLayout, DataDoc::onExportLayout)
    EVT_MENU(ID_SetRecordLength, DataDoc::onSetRecordLength)
    EVT_MENU(ID_SetSkipBytes, DataDoc::onSetSkipBytes)
    EVT_MENU(ID_JumpToPosition, DataDoc::onJumpToPosition)
    EVT_MENU(ID_Fixed, DataDoc::onRectypeChanged)
    EVT_MENU(ID_Variable, DataDoc::onRectypeChanged)
    EVT_MENU(ID_Delimited, DataDoc::onRectypeChanged)
    EVT_UPDATE_UI_RANGE(ID_FirstDataDocCommand, ID_LastDataDocCommand, DataDoc::onUpdateUI)
END_EVENT_TABLE()



DataDoc::DataDoc()
{
    m_buf = new unsigned char[DATA_PAGE_SIZE];
    memset(m_buf, 0, DATA_PAGE_SIZE);
    m_offset = 0;
    m_view = NULL;
    m_view_type = -1;
    m_ebc = false;
    m_caption = _("Data Window");
    
    strcpy(m_dfd.line_delimiter, "\x0a");
    m_dfd.line_delimiter_length = 1;
    m_dfd.record_length = 80;

    m_font = wxFont(10, wxMODERN, wxNORMAL, wxNORMAL);
}


DataDoc::~DataDoc()
{
    if (!m_filename.IsEmpty())
    {
        saveFileInfo();
    }

    if (!m_dfd_filename.IsEmpty())
    {
        m_dfd.save(m_dfd_filename);
    }
}



static wxString getDefaultSaveFilename(const wxString& filename)
{
    wxString fn = filename;
    fn.Replace(wxT("/"), wxT("_"));
    fn.Replace(wxT("\\"), wxT("_"));
    fn.Replace(wxT(":"), wxT("_"));
    fn.Replace(wxT("."), wxT("_"));
    fn.Replace(wxT(" "), wxT("_"));
    wxString path = g_app->getAppDataPath();
    path += PATH_SEPARATOR_STR;
    path += wxT("FileInfo");
    if (!xf_get_directory_exist(towstr(path)))
    {
        xf_mkdir(towstr(path));
    }
    path += PATH_SEPARATOR_STR;
    path += fn;
    path += wxT(".xml");
    return path;
}

wxString getActiveLayoutFile(const wxString& data_file)
{
    kl::xmlnode root;
    
    if (!root.load(towstr(getDefaultSaveFilename(data_file))))
        return wxEmptyString;

    kl::xmlnode& dfd_file_node = root.getChild(L"dfd_file");
    if (dfd_file_node.isEmpty())
    {
        return wxEmptyString;
    }

    return towx(dfd_file_node.getContents());
}

bool setActiveLayoutFile(const wxString& data_file, const wxString& layout_file)
{
    wxString info_file = getDefaultSaveFilename(data_file);
    kl::xmlnode root;
    
    root.load(towstr(info_file));

    if (root.getPropertyIdx(L"version") == -1)
    {
        kl::xmlproperty& prop = root.addProperty();
        prop.name = L"version";
        prop.value = L"1.0";
    }

    root.setTagName(L"kxinter_file_info");

    if (root.getChildIdx(L"dfd_file") != -1)
    {
        kl::xmlnode& dfd_file_node = root.getChild(L"dfd_file");
        dfd_file_node.setNodeValue(towstr(layout_file));
    }
     else
    {
        root.addChild(L"dfd_file", towstr(layout_file));
    }

    return root.save(towstr(info_file));
}


bool DataDoc::saveFileInfo()
{
    kl::xmlnode root;

    kl::xmlproperty& prop = root.addProperty();
    prop.name = L"version";
    prop.value = L"1.0";

    root.setTagName(L"kxinter_file_info");
    root.addChild(L"dfd_file", towstr(m_dfd_filename));

    return root.save(towstr(getDefaultSaveFilename(m_filename)));
}


bool DataDoc::loadFileInfo()
{ 
    kl::xmlnode root;
    
    if (!root.load(towstr(getDefaultSaveFilename(m_filename))))
        return false;

    kl::xmlnode& dfd_file_node = root.getChild(L"dfd_file");
    if (dfd_file_node.isEmpty())
    {
        return false;
    }

    m_dfd_filename = towx(dfd_file_node.getContents());
    
    return true;
}


cfw::IUIContextPtr DataDoc::getUserInterface(cfw::IFramePtr frame)
{
    // -- see if a user interface of this type already exists --
    cfw::IUIContextPtr ui_context;

    ui_context = frame->lookupUIContext(wxT("DataDocUI"));
    if (ui_context.isOk())
        return ui_context;
    
    // -- no, so create one --
    ui_context = frame->createUIContext(wxT("DataDocUI"));

    cfw::wxBitmapMenu* menuView = new cfw::wxBitmapMenu;
    menuView->Append(ID_TextView, _("Text View"), wxNullBitmap, wxEmptyString, true);
    menuView->Append(ID_HexView, _("Hex View"), wxNullBitmap, wxEmptyString, true);
    menuView->AppendSeparator();
    menuView->Append(ID_Ascii, _("ASCII"), wxNullBitmap, wxEmptyString, true);
    menuView->Append(ID_Ebcdic, _("EBCDIC"), wxNullBitmap, wxEmptyString, true);
    menuView->AppendSeparator();
    menuView->Append(ID_Fixed, _("Fixed Length"), wxNullBitmap, wxEmptyString, true);
    menuView->Append(ID_Variable, _("Variable Length"), wxNullBitmap, wxEmptyString, true);
    menuView->Append(ID_Delimited, _("Delimited"), wxNullBitmap, wxEmptyString, true);

    cfw::wxBitmapMenu* menuData = new cfw::wxBitmapMenu;
    menuData->Append(ID_SetActiveLayout, _("Set Active Layout..."));
    menuData->Append(ID_DetachActiveLayout, _("Detach Active Layout"));
    menuData->AppendSeparator();
    menuData->Append(ID_ImportLayout, _("Import Layout..."), GET_XPM(xpm_copybook));
    menuData->Append(ID_ExportLayout, _("Export Layout..."), GET_XPM(xpm_copybook));
    menuData->AppendSeparator();
    menuData->Append(ID_DataFormatEditor, _("Format Editor..."), GET_XPM(xpm_structure));
    menuData->Append(ID_SetRecordLength, _("Set Record Length..."));
    menuData->Append(ID_SetSkipBytes, _("Set Skip Bytes..."));
    menuData->Append(ID_JumpToPosition, _("Jump To Position..."));
    menuData->Append(ID_DefineField, _("Define New Field..."), GET_XPM(xpm_newfield));
    menuData->AppendSeparator();
    menuData->Append(ID_Execute, _("Convert..."), GET_XPM(xpm_execute));

    ui_context->addMenu(menuView, _("View"));
    ui_context->addMenu(menuData, _("Data"));

    // -- add toolbars --
    kcl::ToolBar* toolbar = new kcl::ToolBar(ui_context->getPaneSiteWindow(), 2002, wxPoint(1000,1000), wxSize(0,0), wxNO_BORDER);

    toolbar->AddTool(ID_TextView, GET_XPM(xpm_recordview), wxNullBitmap, true, 0);
    toolbar->AddTool(ID_HexView, GET_XPM(xpm_hexview), wxNullBitmap, true, 0);
    toolbar->AddSeparator();
    toolbar->AddTool(ID_Ascii, GET_XPM(xpm_ascii), wxNullBitmap, true, 0);
    toolbar->AddTool(ID_Ebcdic, GET_XPM(xpm_ebcdic), wxNullBitmap, true, 0);
    toolbar->AddSeparator();
    toolbar->AddTool(ID_DefineField, GET_XPM(xpm_newfield), wxNullBitmap, false, 0);
    toolbar->AddTool(ID_DataFormatEditor, GET_XPM(xpm_structure), wxNullBitmap, false, 0);
    toolbar->AddTool(ID_Execute, GET_XPM(xpm_execute), wxNullBitmap, false, 0);

    toolbar->SetToolShortHelp(ID_TextView, _("Text/Record View"));
    toolbar->SetToolShortHelp(ID_HexView, _("Hex View"));
    toolbar->SetToolShortHelp(ID_DefineField, _("Define a New Field"));
    toolbar->SetToolShortHelp(ID_DataFormatEditor, _("Data Format Editor"));
    toolbar->SetToolShortHelp(ID_Execute, _("Convert"));

    ui_context->addPane(toolbar);

    return ui_context;
}


bool DataDoc::initDoc(cfw::IFramePtr frame, cfw::IDocumentSitePtr doc_site, wxWindow* docsite_wnd, wxWindow* panesite_wnd)
{
    // -- create document's window --
    bool result = Create(docsite_wnd, -1, wxDefaultPosition, docsite_wnd->GetClientSize(), wxCLIP_CHILDREN | wxNO_FULL_REPAINT_ON_RESIZE);
    
    if (!result)
    {
        return false;
    }

    int w, h;
    GetClientSize(&w, &h);

    doc_site->setCaption(m_caption);
    wxSize min_site_size = doc_site->getSiteWindow()->GetSize();
    doc_site->setMinSize(min_site_size.x, min_site_size.y);

    m_doc_site = doc_site;
    
    m_top_ruler = new kcl::RulerControl(this);
    m_top_ruler->SetSize(100,30);

    //m_left_ruler = new kcl::RulerControl(this);

    int ruler_width = 25;
    int client_width, client_height;
    GetClientSize(&client_width, &client_height);


    wxClientDC dc(this);
    dc.SetFont(m_font);
    wxCoord char_width, char_height;
    dc.GetTextExtent(wxT("XXXXXXXXXX"), &char_width, &char_height);

    m_top_ruler->SetSize(0, 0, client_width, ruler_width);
    m_top_ruler->setScale(char_width);

    dc.GetTextExtent(wxT("XXXXXXXX"), &char_width, &char_height);
    m_top_ruler->setDimensions(ruler_width-10, client_width, char_width+30);


    m_status_bar = new wxStatusBar(this, -1);

    m_main_sizer = new wxBoxSizer(wxVERTICAL);
    m_main_sizer->Add(m_top_ruler, 0);
    m_main_sizer->Add(m_status_bar, 0);

    SetSizer(m_main_sizer);

    m_top_ruler->render();
    m_top_ruler->repaint();


    doc_site->setUIContext(getUserInterface(frame));

    setActiveView(viewTextRecord);
    m_view->SetFocus();
    //updateStatusBar();

    return true;
}

wxWindow* DataDoc::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void DataDoc::setDocumentFocus()
{
    if (m_view)
    {
        m_view->SetFocus();
    }
}


bool DataDoc::onSiteClosing(bool force)
{
    if (m_dfd_filename.IsEmpty())
    {
        int result = cfw::appMessageBox(_("Would you like to save the changes you made to the format definition?"),
                                        PRODUCT_NAME, wxYES_NO | wxCANCEL | wxCENTER | wxICON_INFORMATION,
                                       this);

        if (result == wxCANCEL)
        {
            return false;
        }
         else if (result == wxYES)
        {
            wxString filter;
            filter =  _("Layout Files (*.kfl)|*.kfl|All Files (*.*)|*.*|");

            wxFileDialog dlg(g_app->getMainFrame()->getFrameWindow(),
                             _("Export Layout"),
                             wxT(""), wxT(""),
                             filter,
                             wxSAVE | wxHIDE_READONLY);

            if (dlg.ShowModal() == wxID_OK)
            {
                m_dfd_filename = dlg.GetPath();
            }
        }
    }

    return true;
}





void DataDoc::onSize(wxSizeEvent& event)
{
    Layout();
}


void DataDoc::onUpdateUI(wxUpdateUIEvent& event)
{
    switch (event.GetId())
    {
        case ID_Ascii:
            event.Check(!m_ebc);
            break;
        
        case ID_Ebcdic:
            event.Check(m_ebc);
            break;

        case ID_TextView:
            event.Check(m_view_type == viewTextRecord ? true : false);
            break;

        case ID_HexView:
            event.Check(m_view_type == viewHex ? true : false);
            break;

        case ID_DefineField:
            {
                if (m_view_type != viewTextRecord || m_view == NULL)
                {
                    event.Enable(false);
                    break;
                }

                TextView* view = (TextView*)m_view;
                int offset, length, reclen;
                view->getColumnSelection(&offset, &length, &reclen);
                event.Enable(offset != -1 && length > 0 ? true : false);
            }
            break;
        
        case ID_Fixed:
            event.Check(m_dfd.type == dfdFixed ? true : false);
            break;

        case ID_Variable:
            event.Check(m_dfd.type == dfdVariable ? true : false);
            break;

        case ID_Delimited:
            event.Check(m_dfd.type == dfdDelimited ? true : false);
            break;

        case ID_DetachActiveLayout:
            event.Enable(m_dfd_filename.IsEmpty() ? false : true);
            break;

        default:
            event.Enable(true);
            break;
    }
}



unsigned char DataDoc::getByte(int offset)
{
    int off = (offset / DATA_PAGE_SIZE) * DATA_PAGE_SIZE;
    if (off != m_offset)
    {
        m_offset = off;
        m_data->setPosition(m_offset);
        m_data->getData(m_buf, DATA_PAGE_SIZE);
    }

    return m_buf[offset - m_offset];
}

int DataDoc::getRecord(int recno, unsigned char* buf, int max_read, RecordInfo* out_info)
{
    if (m_dfd.type == dfdFixed)
    {
        xf_off_t offset;

        offset = recno;
        offset *= m_dfd.record_length;
        offset += m_dfd.skip_bytes;

        int read = std::min(m_dfd.record_length, max_read);

        if (buf == NULL)
        {
            read = 0;
        }

        m_data->setPosition(offset);

        if (read > 0)
        {
            read = m_data->getData(buf, read);
        }

        out_info->offset = offset;
        out_info->len = read;

        return read;
    }
     else if (m_dfd.type == dfdVariable)
    {
        std::map<int, RecordInfo>::iterator it;
        it = m_records.find(recno);

        if (it != m_records.end())
        {
            m_data->setPosition(it->second.offset);
            m_data->getData(buf, it->second.len);
            memcpy(out_info, &(it->second), sizeof(RecordInfo));
            return out_info->len;
        }
         else
        {
            // -- read variable length data from the beginning --
            ds_off_t off = 0;
            int read_len = 0;
            int block_len;
            int rec_len;
            int buf_off;
            int rec_no = 0;
            bool found = false;

            while (1)
            {
                m_data->setPosition(off);
                read_len = m_data->getData(m_buf, 4);
                if (read_len != 4)
                {
                    out_info->len = 0;
                    return 0;
                }

                block_len = (m_buf[0] << 8) | m_buf[1];
                block_len -= 4;
                read_len = m_data->getData(m_buf, block_len);

                if (read_len != block_len)
                {
                    out_info->len = 0;
                    return 0;
                }

                // -- iterate through records inside the block --
                buf_off = 0;
                while (1)
                {
                    rec_len = (m_buf[buf_off] << 8) | m_buf[buf_off+1];
                    
                    RecordInfo info;
                    info.len = rec_len-4;
                    info.offset = (off+4)+(buf_off+4);

                    m_records[rec_no] = info;


                    if (rec_no == recno)
                    {
                        found = true;
                        int out_len = std::min(info.len, max_read);
                        
                        if (buf)
                        {
                            memmove(buf, m_buf+buf_off+4, out_len);
                        }

                        *out_info = info;
                    }

                    buf_off += rec_len;
                    ++rec_no;

                    if (buf_off >= block_len)
                        break;
                }

                if (found)
                {
                    return out_info->len;
                }

                off += (block_len + 4);
            }
        }

        return 0;
    }
     else if (m_dfd.type == dfdDelimited)
    {
        std::map<int, RecordInfo>::iterator it;
        it = m_records.find(recno);
        if (it != m_records.end())
        {
            m_data->setPosition(it->second.offset);
            m_data->getData(buf, it->second.len);
            memcpy(out_info, &(it->second), sizeof(RecordInfo));
            return out_info->len;
        }
         else
        {
            // -- find the last referenced record --
            int start_row = recno;

            while (start_row > 0)
            {
                start_row--;
                it = m_records.find(start_row);
                if (it != m_records.end())
                    break;
            }

            xf_off_t offset = 0;

            if (start_row > 0)
            {
                offset = it->second.offset;
            }

            // -- find line --
            int read_bytes;

            out_info->offset = 0;
            out_info->len = 0;

            while (start_row <= recno)
            {
                out_info->offset = offset;

                m_data->setPosition(offset);
                read_bytes = m_data->getData(m_buf, DATA_PAGE_SIZE);

                int i;
                bool found = false;

                for (i = 0; i < read_bytes; ++i)
                {
                    if (0 == memcmp(m_buf+i, m_dfd.line_delimiter, m_dfd.line_delimiter_length))
                    {
                        out_info->len = i;
                        offset += (i+m_dfd.line_delimiter_length);
                        found = true;
                        break;
                    }
                }

                if (!found)
                    break;

                start_row++;
            }

            m_records[recno] = *out_info;

            int read = std::min(out_info->len, max_read);
            if (buf == NULL)
            {
                read = 0;
            }
            
            if (read > 0)
            {
                memmove(buf, m_buf, read);
            }

            out_info->len = read;

            return read;
        }
    }

    return 0;
}


bool DataDoc::open(const wxString& filename)
{
    m_data = openFileStream(filename);
    if (m_data.isNull())
        return false;

    m_ebc = false;
    wxString s = filename;
    s.MakeUpper();
    if (s.Find(wxT(".EBC")) != -1)
    {
        m_ebc = true;
    }

    m_filename = filename;

    loadFileInfo();


    if (!m_dfd_filename.IsEmpty())
    {
        m_dfd.load(m_dfd_filename);
    }


    m_data->getData(m_buf, DATA_PAGE_SIZE);

    // -- set window caption --
    m_caption = filename;
    m_caption += wxT(" - ");
    m_caption += _("Data Window");

    return true;
}

void DataDoc::setActiveView(int view_type)
{
    if (view_type == m_view_type)
        return;

    m_view_type = view_type;

    if (m_view)
    {
        m_main_sizer->Detach(m_view);
        m_view->Destroy();
    }

    DataViewBase* view;

    if (view_type == viewTextRecord)
    {
        TextView* v = new TextView(this, this, m_top_ruler, m_status_bar);
        view = (DataViewBase*)v;
    }
     else if (view_type == viewHex)
    {
        HexView* v = new HexView(this, this, m_top_ruler, m_status_bar);
        view = (DataViewBase*)v;
    }

    m_view = view;

    m_main_sizer->Insert(1, m_view, 1, wxEXPAND);
    Layout();
}


int DataDoc::defineField(int record_length, int offset, int length, int type)
{
    RecordDefinition* rec_def = NULL;

    // -- find appropriate record definition --
    std::vector<RecordDefinition>::iterator r_it;
    for (r_it = m_dfd.rec_defs.begin();
         r_it != m_dfd.rec_defs.end(); ++r_it)
    {
        if (r_it->record_length == record_length)
        {
            rec_def = &(*r_it);
            break;
        }
    }

    if (rec_def == NULL)
    {
        if (m_dfd.rec_defs.size() == 1)
        {
            if (m_dfd.rec_defs[0].field_defs.size() == 0)
            {
                m_dfd.rec_defs.erase(m_dfd.rec_defs.begin());
            }
        }


        // -- add a new record definition for specified record length --
        RecordDefinition r;
        r.record_length = record_length;
        r.id_offset = 0;
        r.id_value = 0;
        m_dfd.rec_defs.push_back(r);

        rec_def = &(m_dfd.rec_defs.back());
    }

    FieldDefinition f;
    f.offset = offset;
    f.length = length;
    f.format = m_ebc ? ddformatEBCDIC : ddformatASCII;
    f.type = type;
    f.scale = 0;

    rec_def->field_defs.push_back(f);

    return rec_def->field_defs.size()-1;
}




void DataDoc::onTextView(wxCommandEvent& event)
{
    setActiveView(viewTextRecord);
}


void DataDoc::onHexView(wxCommandEvent& event)
{
    setActiveView(viewHex);
}


void DataDoc::onAscii(wxCommandEvent& event)
{
    m_ebc = false;
    if (m_view)
    {
        m_view->Refresh(false);
    }
}

void DataDoc::onEbcdic(wxCommandEvent& event)
{
    m_ebc = true;
    if (m_view)
    {
        m_view->Refresh(false);
    }
}


void DataDoc::onRectypeChanged(wxCommandEvent& event)
{
    switch (event.GetId())
    {
        case ID_Fixed:
            m_dfd.type = dfdFixed;
            break;

        case ID_Variable:
            m_dfd.type = dfdVariable;
            break;

        case ID_Delimited:
            m_dfd.type = dfdDelimited;
            break;
    }

    m_records.clear();

    if (m_view)
    {
        m_view->Refresh(false);
    }
}


void DataDoc::onDataFormatEditor(wxCommandEvent& event)
{
    if (m_dfd.rec_defs.size() == 0)
    {
        // -- add a new record definition for specified record length --
        RecordDefinition r;
        r.record_length = m_dfd.record_length;
        r.id_offset = 0;
        r.id_value = 0;
        m_dfd.rec_defs.push_back(r);
    }

    showDataFormatPanel();
}

cfw::IDocumentSitePtr DataDoc::showDataFormatPanel()
{
    cfw::IDocumentSitePtr site;
    site = g_app->getMainFrame()->lookupSite(wxT("DataFormatPanel"));

    if (site.isNull())
    {
        DataFormatPanel* p = new DataFormatPanel;
        p->setDataFormat(&m_dfd);
        site = g_app->getMainFrame()->createSite(p, cfw::sitetypeModeless, -1, -1, 550, 400);
        site->setName(wxT("DataFormatPanel"));
    }
     else
    {
        if (!site->getVisible())
        {
            site->setVisible(true);
        }
    }

    return site;
}



void DataDoc::onDefineField(wxCommandEvent& event)
{
    if (m_view_type != viewTextRecord)
        return;

    TextView* view = (TextView*)m_view;

    int offset, length, select_reclen;
    view->getColumnSelection(&offset, &length, &select_reclen);
    if (offset == -1)
        return;

    int idx = defineField(select_reclen, offset, length, 0);

    view->clearColumnSelection();
    view->refresh();

    cfw::IDocumentSitePtr site = showDataFormatPanel();
    if (site)
    {
        IDataFormatPanelPtr d = site->getDocument();
        if (d.isOk())
        {
            d->refresh();

            int recdef_idx = 0;

            std::vector<RecordDefinition>::iterator r_it;
            for (r_it = m_dfd.rec_defs.begin();
                 r_it != m_dfd.rec_defs.end(); ++r_it)
            {
                if (r_it->record_length == select_reclen)
                {
                    break;
                }

                recdef_idx++;
            }

            d->setActivePage(recdef_idx);
            d->editFieldName(idx);
        }
    }
}


void DataDoc::setRecordLength(int new_val)
{
    if (m_dfd.type == dfdFixed)
    {
        m_dfd.record_length = new_val;
    }

    //updateStatusBar();
}

int DataDoc::getRecordLength()
{
    return m_dfd.record_length;
}

void DataDoc::updateStatusBar()
{
    m_status_bar->SetStatusText(wxString::Format(wxT("Record Length: %d"), m_dfd.record_length));
}


void DataDoc::onExecute(wxCommandEvent& event)
{
    ConvertDataJob* job = new ConvertDataJob;

    IConnectionPtr conn = g_app->getConnectionMgr()->getActiveConnection();

    if (!conn)
    {
        cfw::appMessageBox(_("In order to convert data, you must first set an active connection."),
                      PRODUCT_NAME,
                      wxOK | wxCENTER | wxICON_EXCLAMATION,
                      g_app->getMainWindow());
        return;
    }

    if (!conn->isOpen())
    {
        if (!conn->open())
        {
            cfw::appMessageBox(_("The active connection could not be opened."),
                          PRODUCT_NAME,
                          wxOK | wxCENTER | wxICON_EXCLAMATION,
                          g_app->getMainWindow());
            return;
        }
    }


    wxString dest_table;

    if (m_dfd.rec_defs.size() == 1)
    {
        dest_table = m_dfd.rec_defs[0].name;
    }


    if (dest_table.IsEmpty())
    {
        wxTextEntryDialog dlg(g_app->getMainWindow(), _("Please enter the destination table:"));
        if (dlg.ShowModal() != wxID_OK)
            return;

        if (dlg.GetValue().IsEmpty())
        {
            cfw::appMessageBox(_("The table name entered was invalid."),
                          PRODUCT_NAME,
                          wxOK | wxCENTER | wxICON_EXCLAMATION,
                          g_app->getMainWindow());

            return;
        }

        dest_table = dlg.GetValue();
    }


    job->addInstruction(m_data, m_dfd, conn->getDatabasePtr(), dest_table);

    g_app->getJobQueue()->addJob(job, cfw::jobStateRunning);
    g_app->getAppController()->showJobManager();
}


void DataDoc::onSetRecordLength(wxCommandEvent& event)
{
    wxTextEntryDialog dlg(g_app->getMainWindow(), _("Please enter the new record length:"));
    dlg.SetValue(wxString::Format(wxT("%d"), m_dfd.record_length));
    if (dlg.ShowModal() != wxID_OK)
        return;

    int len = wxAtoi(dlg.GetValue().c_str());

    setRecordLength(len);
    
    
    if (m_view)
    {
        m_view->Refresh(false);
    }
}

void DataDoc::onSetSkipBytes(wxCommandEvent& event)
{
    wxTextEntryDialog dlg(g_app->getMainWindow(), _("Please enter the new skip byte count:"));
    dlg.SetValue(wxString::Format(wxT("%d"), m_dfd.skip_bytes));
    if (dlg.ShowModal() != wxID_OK)
        return;

    m_dfd.skip_bytes = wxAtoi(dlg.GetValue().c_str());

    if (m_view)
    {
        m_view->Refresh(false);
    }
}

void DataDoc::onJumpToPosition(wxCommandEvent& event)
{
    wxTextEntryDialog dlg(g_app->getMainWindow(), _("Please enter the new position:"));
    if (dlg.ShowModal() != wxID_OK)
        return;

    int off = wxAtoi(dlg.GetValue().c_str());

    if (m_view_type == viewTextRecord)
    {
        TextView* view = (TextView*)m_view;
        view->setOffset(off);
    }
     else if (m_view_type == viewHex)
    {
        HexView* view = (HexView*)m_view;
        view->setOffset(off);
    }
}



void DataDoc::onSetActiveLayout(wxCommandEvent& event)
{
    wxString filter;
    filter =  _("Layout Files (*.kfl)|*.kfl|All Files (*.*)|*.*|");

    wxFileDialog dlg(g_app->getMainFrame()->getFrameWindow(),
                     _("Set Active Layout"),
                     wxT(""), wxT(""),
                     filter,
                     wxHIDE_READONLY);

    if (dlg.ShowModal() == wxID_OK)
    {
        m_dfd_filename = dlg.GetPath();
        saveFileInfo();

        m_dfd.load(m_dfd_filename);

        if (m_view)
        {
            m_view->Refresh(false);
        }

    }
}


void DataDoc::onDetachActiveLayout(wxCommandEvent& event)
{
    m_dfd_filename = wxT("");
    saveFileInfo();

    cfw::appMessageBox(_("The active layout has been detached.  This data file now has no associated layout."),
                  PRODUCT_NAME,
                  wxOK | wxCENTER,
                  g_app->getMainWindow());
}


void DataDoc::onImportLayout(wxCommandEvent& event)
{
    wxString filter;
    filter =  _("All Layout Files (*.kfl,*.txt,*.cop,*.cob)|*.kfl;*.txt;*.cop;*.cob|COBOL Copy Books (*.txt,*.cop,*.cob)|*.txt;*.cop;*.cob|All Files (*.*)|*.*|");

    wxFileDialog dlg(g_app->getMainFrame()->getFrameWindow(),
                     _("Import Copybook/Layout"),
                     wxT(""), wxT(""),
                     filter,
                     wxOPEN | wxFILE_MUST_EXIST | wxHIDE_READONLY);

    if (dlg.ShowModal() == wxID_OK)
    {
        wxString path = dlg.GetPath();
        wxString ext = path.AfterLast(wxT('.'));
        ext.MakeUpper();

        if (ext == wxT("KFL"))
        {
            int result = cfw::appMessageBox(_("This will overwrite your current data layout definition.  Would you like to continue?"),
                           PRODUCT_NAME, wxYES_NO | wxCENTER | wxICON_INFORMATION,
                           this);
            if (result != wxYES)
                return;

            m_dfd.load(path);
        }
         else
        {
            CopyBookParser p(&m_dfd);

            if (!p.init(dlg.GetPath().mbc_str()))
            {
                // -- error --
                return;
            }

            p.parse();
        }

        showDataFormatPanel();
    }
}


void DataDoc::onExportLayout(wxCommandEvent& event)
{
    wxString filter;
    filter =  _("Layout Files (*.kfl)|*.kfl|All Files (*.*)|*.*|");

    wxFileDialog dlg(g_app->getMainFrame()->getFrameWindow(),
                     _("Export Layout"),
                     wxT(""), wxT(""),
                     filter,
                     wxSAVE | wxHIDE_READONLY);

    if (dlg.ShowModal() == wxID_OK)
    {
        m_dfd.save(dlg.GetPath());
    }
}



