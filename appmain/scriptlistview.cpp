/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-12-22
 *
 */


#include "appmain.h"
#include "scriptapp.h"
#include "scriptgui.h"
#include "scriptbitmap.h"
#include "scriptlistview.h"


// ListViewItem class implementation

// (CLASS) ListViewItem
// Category: Control
// Description: A class that represents an item in a list view control.
// Remarks: The ListViewItem class represents an item in a list view control.

ListViewItem::ListViewItem()
{
    m_owner = NULL;
    m_id = -1;
    m_text = wxEmptyString;
}

ListViewItem::~ListViewItem()
{

}

// (CONSTRUCTOR) ListViewItem.constructor
// Description: Creates a new ListViewItem.
//
// Syntax: ListViewItem(label : String,
//                      bitmap : Bitmap)
//
// Remarks: Creates a new ListViewItem.
//
// Param(label): The text that appears on the list item.
// Param(bitmap): The image that appears on the list item.

void ListViewItem::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() > 0)
        m_text = towx(env->getParam(0)->getString());

    if (env->getParamCount() > 1)
    {
        kscript::ValueObject* obj = env->getParam(1)->getObject();
        if (obj->isKindOf(L"Bitmap"))
        {
            Bitmap* b = (Bitmap*)obj;
            m_bitmap = b->getWxBitmap();
        }
    }
}

// (METHOD) ListViewItem.setText
// Description: Sets the label text of a list view item
//
// Syntax: function ListViewItem.setText(text : String) : Boolean
//
// Remarks: Sets the label text of a list view item
//
// Param(text): The new text of the list view item
//
// Returns: True if the item text was properly set, false otherwise

void ListViewItem::setText(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
    
    int idx = getIndexInternal();
    if (idx != -1)
    {
        if (env->getParamCount() > 0)
            m_text = towx(env->getParam(0)->getString());
             else
            m_text = wxT("");

        if (m_owner && m_owner->isControlValid())
            m_owner->m_ctrl->SetItemText(idx, m_text);

        retval->setBoolean(true);
    }
}

// (METHOD) ListViewItem.getText
// Description: Retrieves the label text of a list view item
//
// Syntax: function ListViewItem.getText() : String
//
// Remarks: Retrieves the label text of a list view item
//
// Returns: A string containing the text of the item's label

void ListViewItem::getText(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setString(towstr(m_text));
}


// (METHOD) ListViewItem.setBitmap
// Description: Sets the bitmap of a list view item
//
// Syntax: function ListViewItem.setBitmap(bitmap : Bitmap) : Boolean
//
// Remarks: Sets the bitmap of a list view item
//
// Param(Bitmap): The new bitmap of the list view item
//
// Returns: True if the bitmap was properly set, false otherwise

void ListViewItem::setBitmap(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
    
    if (env->getParamCount() > 0 && env->getParam(0)->isObject())
    {
        kscript::ValueObject* obj = env->getParam(0)->getObject();
        if (obj->isKindOf(L"Bitmap"))
        {
            Bitmap* b = (Bitmap*)obj;
            m_bitmap = b->getWxBitmap();
        }
    }     

    if (m_owner && m_owner->isControlValid())
    {
        int idx = getIndexInternal();
        if (idx != -1)
        {
            if (!m_bitmap.IsOk() && !m_owner->m_imagelist)
            {
                // there are no images right now, so don't change that
                retval->setBoolean(true);
                return;
            }
            
            wxListItem info;
            info.m_mask = wxLIST_MASK_IMAGE;
            info.m_itemId = idx;
            info.m_col = 0;
            info.m_image = m_owner->getImageListIdx(m_bitmap);
            m_owner->m_ctrl->SetItem(info);
        }
    }

    retval->setBoolean(true);
}


// (METHOD) ListViewItem.getBitmap
// Description: Retrieves the bitmap image of a list view item
//
// Syntax: function ListViewItem.getBitmap() : Bitmap
//
// Remarks: Retrieves the bitmap of a list view item
//
// Returns: A bitmap object containing the item's image

void ListViewItem::getBitmap(kscript::ExprEnv* env, kscript::Value* retval)
{
    Bitmap* bitmap_obj = Bitmap::createObject(env);
    bitmap_obj->setWxBitmap(m_bitmap);
    retval->setObject(bitmap_obj);
}



// (METHOD) ListViewItem.setColumnText
// Description: Sets the text of a list view subitem.
//
// Syntax: function ListView.setColumnText(column_index : Integer, 
//                                         text : String) : Boolean
//
// Remarks: Calling setColumnText() sets the text of a list view subitem.
//     Subitem text values are only displayed in Detail view mode.  The
//     |column_index| parameter is zero-based, meaning that passing zero to
//     this parameter will change the item's label text.
//
// Param(column_index): The zero-based index of the column to set
//
// Returns: True if the item text was properly set, false otherwise


void ListViewItem::setColumnText(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
        
    if (env->getParamCount() < 2)
        return;
    
    int idx = env->getParam(0)->getInteger();
    wxString text = towx(env->getParam(1)->getString());
    
    if (idx < 0)
        return;
    
    if (idx == 0)
    {
        m_text = text;

        if (m_owner && m_owner->isControlValid())
            m_owner->m_ctrl->SetItemText(getIndexInternal(), m_text);

        retval->setBoolean(true);
        return;
    }
    
    if ((size_t)idx > m_columns.size())
        m_columns.resize(idx);

    m_columns[idx-1] = text;
    
    if (m_owner && m_owner->isControlValid())
        m_owner->m_ctrl->SetItem(getIndexInternal(), idx, text);
}


wxString ListViewItem::getTextInternal()
{
    return m_text;
}

void ListViewItem::setTextInternal(const wxString& text)
{
    m_text = text;
}


size_t ListViewItem::getColumnCountInternal()
{
    return m_columns.size()+1;
}


wxString ListViewItem::getColumnTextInternal(int col_idx)
{
    if (col_idx < 0)
        return wxEmptyString;
    
    if (col_idx == 0)
        return m_text;
    
    if ((size_t)(col_idx-1) >= m_columns.size())
        return wxEmptyString;
        
    return m_columns[col_idx-1];
}



// (METHOD) ListViewItem.getColumnText
// Description: Retrieves the subitem text of a list view item
//
// Syntax: function ListView.getColumnText(column_index : Integer) : String
//
// Remarks: Retrieves the subitem text of a list view item. The
//     subitem text values are only displayed in Detail view mode.
//     |column_index| parameter is zero-based, meaning that passing zero to
//     this parameter will retrieve the item's normal label text.
//
// Param(column_index): The zero-based index of the column to get
//
// Returns: A string containing the text of the item's subitem label

void ListViewItem::getColumnText(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setNull();
        return;
    }
    
    retval->setString(towstr(getColumnTextInternal(env->getParam(0)->getInteger())));
}

// (METHOD) ListViewItem.getIndex
// Description: Retrieves the index of a list view item
//
// Syntax: function ListView.getIndex() : Integer
//
// Remarks: Retrieves the zero-based index of an item inside
//     a list view control
//
// Returns: The index of the list view item

void ListViewItem::getIndex(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setInteger(getIndexInternal());
}



int ListViewItem::getIndexInternal()
{
    if (!m_owner)
        return -1;

    if (!m_owner->isControlValid())
        return -1;

    return m_owner->m_ctrl->FindItem(-1, (long)this);
}



// ListView class implementation

// (CLASS) ListView
// Category: Control
// Derives: FormControl
// Description: A class that represents a list view control.
// Remarks: The ListView class represents a list view control.
//
// Property(ListView.LargeIcon):   A flag representing that the ListView should be created in large icon mode.
// Property(ListView.SmallIcon):   A flag representing that the ListView should be created in small icon mode.
// Property(ListView.Details):     A flag representing that the ListView should be created in details mode (single or multiple column list with column headers).
// Property(ListView.List):        A flag representing that the ListView should be created in list mode (single or multiple column list without column headers).
// Property(ListView.AlignLeft):   A flag representing that a column in the ListView should be left-aligned (ListView.Details and ListView.List modes only).
// Property(ListView.AlignCenter): A flag representing that a column in the ListView should be center-aligned (ListView.Details and ListView.List modes only).
// Property(ListView.AlignRight):  A flag representing that a column in the ListView should be right-aligned (ListView.Details and ListView.List modes only).

// (EVENT) ListView.columnClick
// Description: Fired when a column is left-clicked.
// Param(index): The index of the column that is clicked.

// (EVENT) ListView.columnRightClick
// Description: Fired when a column is right-clicked.
// Param(index): The index of the column that is clicked.

// (EVENT) ListView.keyDown
// Description: Fired when a key is pressed.
// Param(index): Index of the list item that has the focus.
// Param(keyCode): Key code of the key pressed.
// Param(altDown): True if the Alt key is pressed, false otherwise.
// Param(controlDown): True if the Control key is pressed, false otherwise.
// Param(shiftDown): True if the Shift key is pressed, false otherwise.

// (EVENT) ListView.itemSelect
// Description: Fired when a list item is selected.
// Param(item): The item that is selected.
// Param(index): The index of the item that is selected.

// (EVENT) ListView.itemActivate
// Description: Fired when a list item is activated.
// Param(item): The item that is activated.
// Param(index): The index of the item that is activated.

// (EVENT) ListView.itemRightClick
// Description: Fired when a list item is right-clicked.
// Param(item): The item that is clicked.
// Param(index): The index of the item that is clicked.

// (EVENT) ListView.itemBeginLabelEdit
// Description: Fired at the beginning of a list item label edit.
// Param(item): The item that is beginning to be edited.
// Param(index): The index of the item that is beginning to be edited.

// (EVENT) ListView.itemEndLabelEdit
// Description: Fired at the end of a list item label edit.
// Param(item): The item that is finished being edited.
// Param(index): The index of the item that is finished being edited.

ListView::ListView()
{
    m_imagelist = NULL;
    m_ctrl = NULL;
    m_allow_label_edit = true;
}

ListView::~ListView()
{
    std::vector<ListViewItem*>::iterator it;
    for (it = m_items.begin(); it != m_items.end(); ++it)
    {
        (*it)->m_owner = NULL;
        (*it)->baseUnref();
    }
    m_items.clear();
    
    
    if (m_imagelist)
        delete m_imagelist;
}

wxListCtrl* ListView::getWxListCtrl()
{
    return m_ctrl;
}

int ListView::getImageListIdx(const wxBitmap& bitmap)
{
    if (!m_imagelist)
    {
        int w = bitmap.GetWidth();
        int h = bitmap.GetHeight();
        
        if (w == 0 || h == 0)
        {
            w = 16;
            h = 16;
        }
        
        // if our list control doesn't have an imagelist, create one
        m_imagelist = new wxImageList(w, h, true, 0);
        
        // add an empty image as image zero
        wxImage black_img(16, 16, true);
        wxBitmap black_bmp(black_img);
        m_imagelist->Add(black_bmp, *wxBLACK);
        
        m_ctrl->SetImageList(m_imagelist, wxIMAGE_LIST_NORMAL);
        m_ctrl->SetImageList(m_imagelist, wxIMAGE_LIST_SMALL);
    }
            
        
    if (!bitmap.Ok())
        return -1;
    
    int idx = 0;
    int count = m_imagelist->GetImageCount();
    
    for (idx = 0; idx < count; ++idx)
    {
        if (m_imagelist->GetBitmap(idx).IsSameAs(bitmap))
            return idx;
    }
    
    idx = m_imagelist->Add(bitmap);
    return idx;
}

// (CONSTRUCTOR) ListView.constructor
// Description: Creates a new ListView control.
//
// Syntax: ListView(x_pos : Integer,
//                  y_pos : Integer,
//                  width : Integer,
//                  height : Integer)
//
// Remarks: Creates a new ListView control at the position specified by 
//     |x_pos| and |y_pos|, and having dimensions specified by |width| 
//     and |height|.
//
// Param(x_pos): The x position of the control.
// Param(y_pos): The y position of the control.
// Param(width): The width of the control.
// Param(height): The height of the control.

void ListView::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // initialize the component
    initComponent(env);

    // initialize event objects
    getMember(L"keyDown")->setObject(Event::createObject(env));
    getMember(L"itemActivate")->setObject(Event::createObject(env));
    getMember(L"itemRightClick")->setObject(Event::createObject(env));
    getMember(L"itemSelect")->setObject(Event::createObject(env));
    getMember(L"itemBeginLabelEdit")->setObject(Event::createObject(env));
    getMember(L"itemEndLabelEdit")->setObject(Event::createObject(env));
    getMember(L"columnClick")->setObject(Event::createObject(env));
    getMember(L"columnRightClick")->setObject(Event::createObject(env));

    // set default values
    m_x = 0;
    m_y = 0;
    m_width = -1;
    m_height = -1;
    
    size_t param_count = env->getParamCount();
    
    // get user input values
    if (param_count > 0)
        m_x = env->getParam(0)->getInteger();
    if (param_count > 1)
        m_y = env->getParam(1)->getInteger();
    if (param_count > 2)
        m_width = env->getParam(2)->getInteger();
    if (param_count > 3)
        m_height = env->getParam(3)->getInteger();

    if (param_count < 1)
    {
        // create the control
        m_ctrl = new wxListCtrl(getApp()->getTempParent(),
                                -1,
                                wxDefaultPosition,
                                wxDefaultSize,
                                wxLC_ICON |
                                wxLC_EDIT_LABELS);
        m_wnd = m_ctrl;

        // if no sizing parameters were specified, used wx's default
        // parameters, since they are equal to the exact size of the text
        wxSize s = m_ctrl->GetSize();
        m_width = s.GetWidth();
        m_height = s.GetHeight();
    }
     else
    {
        // create the control
        m_ctrl = new wxListCtrl(getApp()->getTempParent(),
                                -1,
                                wxPoint(m_x, m_y),
                                wxSize(m_width, m_height),
                                wxLC_ICON |
                                wxLC_EDIT_LABELS);
        m_wnd = m_ctrl;

        // make sure we update the m_width and m_height member
        // variables based on the actual size of the control
        wxSize s = m_ctrl->GetSize();
        if (m_width != s.GetWidth())
            m_width = s.GetWidth();
        if (m_height != s.GetHeight())
            m_height = s.GetHeight();
    }
}




void ListView::realize()
{
    m_ctrl->Reparent(m_form_wnd);
    
    if (!m_enabled)
        m_ctrl->Enable(false);
    
    listenEvent(wxEVT_COMMAND_LIST_KEY_DOWN);
    listenEvent(wxEVT_COMMAND_LIST_ITEM_SELECTED);
    listenEvent(wxEVT_COMMAND_LIST_ITEM_ACTIVATED);
    listenEvent(wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK);
    listenEvent(wxEVT_COMMAND_LIST_BEGIN_LABEL_EDIT);
    listenEvent(wxEVT_COMMAND_LIST_END_LABEL_EDIT);
    listenEvent(wxEVT_COMMAND_LIST_COL_CLICK);
    listenEvent(wxEVT_COMMAND_LIST_COL_RIGHT_CLICK);
}

// (METHOD) ListView.addItem
// Description: Adds a ListViewItem to the ListView.
//
// Syntax: function ListView.addItem(item : ListViewItem)
//
// Remarks: Adds a ListViewItem |item| to the ListView.
//
// Param(item): The ListViewItem |item| to add to the ListView.

void ListView::addItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;
    
    if (env->getParam(0)->isObject() &&
        env->getParam(0)->getObject()->isKindOf(L"ListViewItem"))
    {
        // if a ListViewItem object was passed
        ListViewItem* item = (ListViewItem*)env->getParam(0)->getObject();
        
        // this item belongs to another ListView, we need
        // to copy it before we add it to this ListView
        if (item->m_owner)
        {
            // create a copy of this item to be added to this ListView
            ListViewItem* new_item = ListViewItem::createObject(env);
            new_item->m_text = item->m_text;
            new_item->m_bitmap = item->m_bitmap;
            
            // copy the columns from the old ListViewItem
            std::vector<wxString> cols = item->m_columns;
            std::vector<wxString>::iterator it;
            for (it = cols.begin(); it != cols.end(); ++it)
                new_item->m_columns.push_back(*it);
            
            // set the item pointer to the new item
            item = new_item;
        }
        
        int bmp_idx = -1;
        if (item->m_bitmap.Ok())
            bmp_idx = getImageListIdx(item->m_bitmap);
        
        // set the item's m_owner variable to this ListView
        item->m_owner = this;
        
        item->m_id = m_ctrl->InsertItem(m_ctrl->GetItemCount(),
                                        item->m_text,
                                        bmp_idx);
        
        size_t col_count = item->getColumnCountInternal();
        for (size_t i = 1; i < col_count; ++i)
        {
            wxListItem info;
            info.m_mask = wxLIST_MASK_TEXT;
            info.m_itemId = item->m_id;
            info.m_col = i;
            info.m_text = item->getColumnTextInternal(i);
            m_ctrl->SetItem(info);
        }
        
        // associate the ListViewItem class with the id so we can
        // retrieve the ListViewItem class when an event is fired
        m_ctrl->SetItemData(item->m_id, (long)item);
        
        // hang on to a reference
        item->baseRef();
        m_items.push_back(item);
    }
}


// (METHOD) ListView.insertItem
// Description: Inserts a ListViewItem into the ListView.
//
// Syntax: function ListView.insertItem(item : ListViewItem, 
//                                      index : Integer)
//
// Remarks: Inserts a ListViewItem |item| into the ListView at the
//     specified position.
//
// Param(item): The ListViewItem |item| to insert into the ListView.
// Param(index): The position of the new item.

void ListView::insertItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 2 || !isControlValid())
        return;
    
    int index = env->getParam(1)->getInteger();
    if (index < 0 || index >= m_ctrl->GetItemCount())
        index = m_ctrl->GetItemCount();
    
    if (env->getParam(0)->isObject() &&
        env->getParam(0)->getObject()->isKindOf(L"ListViewItem"))
    {
        // if a ListViewItem object was passed
        ListViewItem* item = (ListViewItem*)env->getParam(0)->getObject();
        
        // this item belongs to another ListView, we need
        // to copy it before we add it to this ListView
        if (item->m_owner)
        {
            // create a copy of this item to be added to this ListView
            ListViewItem* new_item = ListViewItem::createObject(env);
            new_item->m_text = item->m_text;
            new_item->m_bitmap = item->m_bitmap;
            
            // copy the columns from the old ListViewItem
            std::vector<wxString> cols = item->m_columns;
            std::vector<wxString>::iterator it;
            for (it = cols.begin(); it != cols.end(); ++it)
                new_item->m_columns.push_back(*it);
            
            // set the item pointer to the new item
            item = new_item;
        }
        
        int bmp_idx = -1;
        if (item->m_bitmap.Ok())
            bmp_idx = getImageListIdx(item->m_bitmap);
        
        // set the item's m_owner variable to this ListView
        item->m_owner = this;
        
        item->m_id = m_ctrl->InsertItem(index,
                                        item->m_text,
                                        bmp_idx);
        
        size_t col_count = item->getColumnCountInternal();
        for (size_t i = 1; i < col_count; ++i)
        {
            wxListItem info;
            info.m_mask = wxLIST_MASK_TEXT;
            info.m_itemId = item->m_id;
            info.m_col = i;
            info.m_text = item->getColumnTextInternal(i);
            m_ctrl->SetItem(info);
        }
        
        // associate the ListViewItem class with the id so we can
        // retrieve the ListViewItem class when an event is fired
        m_ctrl->SetItemData(item->m_id, (long)item);
        
        // hang on to a reference
        item->baseRef();
        m_items.insert(m_items.begin() + index, item);
    }
}



// (METHOD) ListView.getItem
// Description: Retrieves a list view item
//
// Syntax: function ListView.getItem(item_index : Integer) : ListViewItem
//
// Remarks: Retrieves an item from the list view.  In the parameter,
//     either an item index or a list view item object may be specified.
//
// Param(item_index): The zero-based index of the item to retrieve
//
// Returns: A ListViewItem object.  If the call failed, |null| is returned.

void ListView::getItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();
    
    if (env->getParamCount() < 1 || !isControlValid())
        return;
    
    int idx = env->getParam(0)->getInteger();
    
    if (idx >= 0 && idx < m_ctrl->GetItemCount())
    {
        ListViewItem* item = (ListViewItem*)m_ctrl->GetItemData(idx);
        retval->setObject(item);
    }
}


// (METHOD) ListView.deleteItem
// Description: Deletes a list view item
//
// Syntax: function ListView.deleteItem(item : ListViewItem)
// Syntax: function ListView.deleteItem(item_index : Integer)
//
// Remarks: Deletes an item from the list view.  Either an
//     item index or a list view item object may be specified
//     in the parameter.
//
// Param(item): A list view item object
// Param(item_index): The zero-based index of the item to delete
//
// Returns: True if the item was properly deleted, false otherwise

void ListView::deleteItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
    
    if (env->getParamCount() < 1 || !isControlValid())
        return;
    
    int idx = -1;
    
    if (env->getParam(0)->isNumber())
    {
        idx = env->getParam(0)->getInteger();
    }
     else if (env->getParam(0)->isObject())
    {
        kscript::ValueObject* obj = env->getParam(0)->getObject();
        if (obj->isKindOf(L"ListViewItem"))
        {
            ListViewItem* item = (ListViewItem*)obj;
            idx = item->getIndexInternal();
        }
         else
        {
            // unknown object type -- return error
            return;
        }
    }
    
    
    if (idx == -1)
        return;
    
    if (idx >= 0 && idx < m_ctrl->GetItemCount())
    {
        ListViewItem* item = (ListViewItem*)m_ctrl->GetItemData(idx);
        m_ctrl->DeleteItem(idx);
        m_items.erase(m_items.begin() + idx);
        item->m_owner = NULL;
        item->baseUnref();
        retval->setBoolean(true);
    }
}

// (METHOD) ListView.deselectItem
// Description: Deselects the specified list view item
//
// Syntax: function ListView.deselectItem(item : ListViewItem)
// Syntax: function ListView.deselectItem(item_index : Integer)
//
// Remarks: Deselects the specified |item| or the item at the specified |index|.
//
// Param(item): The list view item object to deselect
// Param(item_index): The zero-based index of the item to deselect

void ListView::deselectItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;
    
    int idx = -1;
    
    if (env->getParam(0)->isNumber())
    {
        idx = env->getParam(0)->getInteger();
    }
     else if (env->getParam(0)->isObject())
    {
        kscript::ValueObject* obj = env->getParam(0)->getObject();
        if (obj->isKindOf(L"ListViewItem"))
        {
            ListViewItem* item = (ListViewItem*)obj;
            idx = item->getIndexInternal();
        }
         else
        {
            // unknown object type -- return error
            return;
        }
    }
    
    if (idx == -1)
        return;
    
    if (idx >= 0 && idx < m_ctrl->GetItemCount())
    {
        wxListItem info;
        info.m_mask = wxLIST_MASK_STATE;
        info.m_itemId = idx;
        info.m_col = 0;
        info.m_stateMask = wxLIST_STATE_SELECTED;
        info.m_state = 0;
        m_ctrl->SetItem(info);
        m_ctrl->RefreshItem(idx);
    }
}

// (METHOD) ListView.deselectAllItems
//
// Description: Deselects all the items in the list view control
//
// Syntax: function ListView.deselectAllItems()
//
// Remarks: Deselects all the items in the list view control

void ListView::deselectAllItems(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    long idx = -1;
    
    while (1)
    {
        idx = m_ctrl->GetNextItem(idx, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (idx < 0)
            break;
        
        if (idx >= 0 && idx < m_ctrl->GetItemCount())
        {
            wxListItem info;
            info.m_mask = wxLIST_MASK_STATE;
            info.m_itemId = idx;
            info.m_col = 0;
            info.m_stateMask = wxLIST_STATE_SELECTED;
            info.m_state = 0;
            m_ctrl->SetItem(info);
            m_ctrl->RefreshItem(idx);
        }
    }
}

// (METHOD) ListView.findItem
// Description: Finds an item with the specified text.
//
// Syntax: function ListView.findItem(text : String,
//                                    index : Integer) : ListViewItem
//
// Remarks: Finds an item with the specified |text| and returns
//     the ListViewItem that was found.  If a ListViewItem with the
//     specified |text| can't be found, the function returns null.
//
// Param(text): The |text| of the item to find.
// Param(index): (Optional) The zero-based |index| of the item to start at.
//     If this parameter is left blank or is -1, the find operation
//     will always start with the first item in the ListView.
//
// Returns: Returns the ListViewItem if it's found, and
//     null if the item can't be found.

void ListView::findItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }
    
    retval->setNull();
    
    size_t param_count = env->getParamCount();
    if (param_count < 1)
        return;
    
    // get the find string
    wxString s = towx(env->getParam(0)->getString());
    
    // get the start index
    int start_idx = -1;
    if (param_count > 1)
        start_idx = env->getParam(1)->getInteger();
    
    // don't allow negative start indexes less than -1
    if (start_idx < -1)
        return;
    
    // the start index is exclusive, not inclusive,
    // so we need to increment this variable here (-1 will start
    // matching at index 0; 0 will start matching at index 1, etc.)
    start_idx++;
    
    // index out-of-bounds, bail out
    int item_count = m_ctrl->GetItemCount();
    if (start_idx < 0 || start_idx >= item_count)
        return;
    
    // try to find the item
    long idx = m_ctrl->FindItem(start_idx, s);
    
    // if we found the item, return it
    if (idx >= 0 && idx < item_count)
    {
        ListViewItem* item = (ListViewItem*)m_ctrl->GetItemData(idx);
        retval->setObject(item);
    }
}

// (METHOD) ListView.clear
// Description: Deletes all items from a list view control
//
// Syntax: function ListView.clear()
//
// Remarks: Deletes all list view items from the control, clearing it out.
//     Columns are not removed.

void ListView::clear(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    m_ctrl->DeleteAllItems();
    
    // release stored references to items
    std::vector<ListViewItem*>::iterator it;
    for (it = m_items.begin(); it != m_items.end(); ++it)
    {
        (*it)->m_owner = NULL;
        (*it)->baseUnref();
    }
    m_items.clear();
}

// (METHOD) ListView.getItemCount
// Description: Returns the number of items in the list view
//
// Syntax: function ListView.getItemCount() : Integer
//
// Remarks: Returns the number of items in the list view control
//
// Returns: The number of items in the list view control


void ListView::getItemCount(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    retval->setInteger(m_ctrl->GetItemCount());
}



// (METHOD) ListView.getItems
// Description: Returns all ListViewItem objects in the list view control
//
// Syntax: function ListView.getItems() : Array(ListViewItem)
//
// Remarks: Returns an Array object containing all ListViewItem objects
//     in the list view control
//
// Returns: An Array of ListViewItem objects

void ListView::getItems(kscript::ExprEnv* env, kscript::Value* retval)
{
    // set our return type to be an array
    retval->setArray(env);
    
    if (!isControlValid())
        return;

    long idx = -1;
    
    while (1)
    {
        idx = m_ctrl->GetNextItem(idx, wxLIST_NEXT_ALL, wxLIST_STATE_DONTCARE);
        if (idx < 0)
            break;
        
        ListViewItem* item = (ListViewItem*)m_ctrl->GetItemData(idx);
        if (item)
        {
            kscript::Value val;
            val.setObject(item);
            retval->appendMember(&val);
        }
    }
}


// (METHOD) ListView.getSelectedIndex
// Description: Gets the index of the selected item in the listbox.
//
// Syntax: function ListView.getSelectedIndex(index : Integer) : Integer
//
// Remarks: Returns the zero-based index of the first selected ListViewItem
//          object in the list view control after the start |index|.
//          If there are no selected items, the function returns -1.
//
// Param(index): (Optional) The zero-based |index| of the item to start at.
//     If this parameter is left blank or is -1, the find operation
//     will always start with the first item in the list view control.
//
// Returns: Returns the zero-based index of the first selected ListViewItem
//          object in the list view control after the start |index|.
//          If there are no selected items, the function returns -1.

void ListView::getSelectedIndex(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    retval->setInteger(-1);
    
    int param_count = env->getParamCount();
    
    // get the start index
    int start_idx = -1;
    if (param_count > 0)
        start_idx = env->getParam(0)->getInteger();
    
    long idx = m_ctrl->GetNextItem(start_idx, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    retval->setInteger((int)idx);
}

// (METHOD) ListView.getSelectedItem
// Description: Returns the first selected ListViewItem object
//              after the specified index in the list view control
//
// Syntax: function ListView.getSelectedItem(index : Integer) : ListViewItem
//
// Remarks: Returns the first selected ListViewItem object in the list view control.
//     If no objects are selected in the list view control, null is returned.
//
// Param(index): (Optional) The zero-based |index| of the item to start at.
//     If this parameter is left blank or is -1, the find operation
//     will always start with the first item in the list view control.
//
// Returns: A ListViewItem object, or null if no items are selected in the list view.

void ListView::getSelectedItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    retval->setNull();
    
    int param_count = env->getParamCount();
    
    // get the start index
    int start_idx = -1;
    if (param_count > 0)
        start_idx = env->getParam(0)->getInteger();
    
    long idx = m_ctrl->GetNextItem(start_idx, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    
    if (idx >= 0)
    {
        ListViewItem* item = (ListViewItem*)m_ctrl->GetItemData(idx);
        if (item)
            retval->setObject(item);
    } 
}

// (METHOD) ListView.getSelectedItems
// Description: Returns the selected ListViewItem objects in the list view control
//
// Syntax: function ListView.getSelectedItems() : Array(ListViewItem)
//
// Remarks: Returns an Array object containing all selected ListViewItem objects
//     in the list view control.  If no objects are selected in the list view control,
//     an empty array is returned.
//
// Returns: An Array of ListViewItem objects

void ListView::getSelectedItems(kscript::ExprEnv* env, kscript::Value* retval)
{
    // set our return type to be an array
    retval->setArray(env);
    
    if (!isControlValid())
        return;

    long idx = -1;
    
    while (1)
    {
        idx = m_ctrl->GetNextItem(idx, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (idx < 0)
            break;
        
        ListViewItem* item = (ListViewItem*)m_ctrl->GetItemData(idx);
        if (item)
        {
            kscript::Value val;
            val.setObject(item);
            retval->appendMember(&val);
        }
    }
}

// (METHOD) ListView.isItemSelected
// Description: Indicates whether or not the specified ListViewItem
//              object is selected
//
// Syntax: function ListView.isItemSelected(item : ListViewItem) : Boolean
//
// Remarks: Indicates whether or not the specified |item| is
//     selected. If the item is selected, the function returns true.
//     If the item is not selected, the function returns false.
//
// Param(index): The |item| that is being checked for selection.
//
// Returns: Returns true if the |item| is selected, and false otherwise.
//          If an error is encountered during the call, null is returned.

void ListView::isItemSelected(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }
    
    retval->setBoolean(false);
    
    // not enough parameters specified
    if (env->getParamCount() < 1)
        return;
    
    int idx = -1;
    
    // get the index of the ListViewItem that was passed as a parameter
    if (env->getParam(0)->isObject())
    {
        kscript::ValueObject* obj = env->getParam(0)->getObject();
        if (obj->isKindOf(L"ListViewItem"))
        {
            ListViewItem* item = (ListViewItem*)obj;
            idx = item->getIndexInternal();
        }
         else
        {
            // unknown object type -- return error
            return;
        }
    }
    
    // bad index, bail out
    if (idx == -1)
        return;
    
    // get the state of the item
    long state = m_ctrl->GetItemState(idx, wxLIST_STATE_SELECTED);
    bool selected = (state & wxLIST_STATE_SELECTED) ? true : false;
    retval->setBoolean(selected);
}




// (METHOD) ListView.addColumn
// Description: Adds a column to the ListView
//
// Syntax: function ListView.addColumn(caption : String, 
//                                     width : Integer, 
//                                     alignment : AlignmentEnum)
//
// Remarks: Adds a column to the list view control.  The list view must be
//     in Details mode in order for the column to be displayed.  See setView() for
//     more information.
//
// Param(caption): The column caption
// Param(width): The width of the column in pixels
// Param(alignment): The column alignment (ListView.AlignLeft,
//     ListView.AlignCenter, or ListView.AlignRight)
//
// Returns: True if the column was properly added, false otherwise

void ListView::addColumn(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    std::wstring col_text;
    int col_width = 100;
    int col_align = AlignLeft;
    
    if (env->getParamCount() >= 1)
        col_text = env->getParam(0)->getString();
    
    if (env->getParamCount() >= 2)
        col_width = env->getParam(1)->getInteger();
        
    if (env->getParamCount() >= 3)
        col_align = env->getParam(2)->getInteger();
    
    int wx_col_align;
    switch (col_align)
    {
        case AlignLeft: wx_col_align = wxLIST_FORMAT_LEFT; break;
        case AlignCenter: wx_col_align = wxLIST_FORMAT_CENTER; break;
        case AlignRight: wx_col_align = wxLIST_FORMAT_RIGHT; break;
        default:
            retval->setBoolean(false);
            return;
    }
       
    m_ctrl->InsertColumn(m_ctrl->GetColumnCount(), towx(col_text), wx_col_align, col_width);
    
    retval->setBoolean(true);
}



// (METHOD) ListView.deleteColumn
// Description: Deletes a column from the ListView
//
// Syntax: function ListView.deleteColumn(column_index : Integer)
//
// Remarks: Deletes a column from the list view control. 
//
// Param(column_index): The zero-based column to delete.
//
// Returns: True if the column was properly added, false otherwise

void ListView::deleteColumn(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
    {
        retval->setBoolean(false);
        return;
    }
    
    int idx = env->getParam(0)->getInteger();
    
    if (idx < 0 || idx >= m_ctrl->GetColumnCount())
    {
        // invalid column index
        retval->setBoolean(false);
        return;
    }
    
    retval->setBoolean(m_ctrl->DeleteColumn(idx));
}


// (METHOD) ListView.setView
// Description: Sets the view mode of the list view control
//
// Syntax: function ListView.setView(mode : ListViewModeEnum) : Boolean
//
// Remarks: List view controls have several view modes which allow
//     data to be represented in various ways on the screen.  The modes
//     are ListView.LargeIcon, ListView.SmallIcon, ListView.Details, and
//     ListView.List.  The ListView.LargeIcon and ListView.SmallIcon modes
//     allow items with various icon sizes to be viewed in a window.  The
//     ListView.Details and ListView.List modes allow items to be viewed as
//     a list, along with extra columns to displayed next to the main item.
//     The ListView.Details mode, in contrast with the ListView.List mode,
//     has column headers.  To use the ListView.Details or ListView.List
//     mode, the caller must first add columns to the control.
//
// Param(mode): One of the following values: ListView.LargeIcon, ListView.SmallIcon,
//     ListView.Details, or ListView.List
//
// Returns: True if the view mode was successfully set, false otherwise


void ListView::setView(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
    {
        retval->setBoolean(false);
        return;
    }
    
    bool result = true;
    
    long style = m_ctrl->GetWindowStyleFlag();
    style &= ~(wxLC_LIST | wxLC_REPORT | wxLC_ICON | wxLC_SMALL_ICON);
    
    switch (env->getParam(0)->getInteger())
    {
        case LargeIcon: style |= wxLC_ICON;         break;
        case SmallIcon: style |= wxLC_SMALL_ICON;   break;
        case Details:   style |= wxLC_REPORT;       break;
        case List:      style |= wxLC_LIST;         break;
        default:
            result = false;
            break;
    }
    
    m_ctrl->SetWindowStyleFlag(style);
    m_ctrl->Refresh();
    
    retval->setBoolean(result);
}


// (METHOD) ListView.getView
// Description: Returns the current view mode of the list view control.
//
// Syntax: function ListView.getView() : ListViewModeEnum
//
// Remarks: Returns the current view mode of the list view control.
//     See setView() for more information.
//
// Returns: One of the following values: ListView.LargeIcon, ListView.SmallIcon,
//     ListView.Details, or ListView.List

void ListView::getView(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    long style = m_ctrl->GetWindowStyleFlag();
    
    if (style & wxLC_ICON)
        retval->setInteger(LargeIcon);
    else if (style & wxLC_SMALL_ICON)
        retval->setInteger(SmallIcon);
    else if (style & wxLC_REPORT)
        retval->setInteger(Details);
    else if (style & wxLC_LIST)
        retval->setInteger(List);
    else retval->setInteger(0);
}



// (METHOD) ListView.setLabelEdit
// Description: Sets whether the item labels may be edited
//
// Syntax: function ListView.setLabelEdit(allow_label_edit : Boolean)
//
// Remarks: Invoking setLabelEdit allows the caller to determine whether label
//     edits are generally allowed in the list view control.  Passing
//     |true| to the |allow_label_edit| parameter effectively makes the control
//     read-only from the user's perspective.


void ListView::setLabelEdit(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    if (env->getParamCount() > 0)
    {
        m_allow_label_edit = env->getParam(0)->getBoolean();
    }
     else
    {
        m_allow_label_edit = true;
    }
}


// (METHOD) ListView.getLabelEdit
// Description: Returns whether the item labels may be edited
//
// Syntax: function ListView.getLabelEdit() : Boolean
//
// Remarks: Returns a boolean value indicated whether the list view item
//     labels may be edited
//
// Returns: |true| if item labels may be edited by the user, |false| otherwise

void ListView::getLabelEdit(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    retval->setBoolean(m_allow_label_edit);
}

// (METHOD) ListView.selectItem
// Description: Selects the specified list view item
//
// Syntax: function ListView.selectItem(item : ListViewItem)
// Syntax: function ListView.selectItem(item_index : Integer)
//
// Remarks: Selects the specified |item| or the item at the specified |index|.
//
// Param(item): The list view item object to select
// Param(item_index): The zero-based index of the item to select

void ListView::selectItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;
    
    int idx = -1;
    
    if (env->getParam(0)->isNumber())
    {
        idx = env->getParam(0)->getInteger();
    }
     else if (env->getParam(0)->isObject())
    {
        kscript::ValueObject* obj = env->getParam(0)->getObject();
        if (obj->isKindOf(L"ListViewItem"))
        {
            ListViewItem* item = (ListViewItem*)obj;
            idx = item->getIndexInternal();
        }
         else
        {
            // unknown object type -- return error
            return;
        }
    }
    
    if (idx == -1)
        return;
    
    if (idx >= 0 && idx < m_ctrl->GetItemCount())
    {
        wxListItem info;
        info.m_mask = wxLIST_MASK_STATE;
        info.m_itemId = idx;
        info.m_col = 0;
        info.m_stateMask = wxLIST_STATE_SELECTED;
        info.m_state = wxLIST_STATE_SELECTED;
        m_ctrl->SetItem(info);
        m_ctrl->RefreshItem(idx);
    }
}

// (METHOD) ListView.sortItems
// Description: Sorts the items in the list view control
//
// Syntax: function ListView.sortItems(column_index : Integer, 
//                                     ascending : Boolean)
//
// Remarks: Sorts the items in the list view.  The |column_index| parameter specifies
//     the column index to use in the sort.  The second parameter, which is optional,
//     specifies the sort order.  If this parameter is not specified, an ascending
//     sort order is assumed.
//
// Param(column_index): The zero-based column index used as a basis for the sort operation
// Param(ascending): (Optional) Specifying |true| directs the operation to sort the control
//     ascending, |false| descending.


struct SortHelperInfo
{
    int col_idx;
    bool ascending;
};

#if wxCHECK_VERSION(2,9,3)
static int wxCALLBACK sortHelper(wxIntPtr _item1, wxIntPtr _item2, wxIntPtr _sortData)
#elif wxCHECK_VERSION(2,9,0)
static int wxCALLBACK sortHelper(long _item1, long _item2, wxIntPtr _sortData)
#else
static int wxCALLBACK sortHelper(long _item1, long _item2, long _sortData)
#endif
{
    ListViewItem* item1 = (ListViewItem*)_item1;
    ListViewItem* item2 = (ListViewItem*)_item2;
    SortHelperInfo* info = (SortHelperInfo*)_sortData;
    
    wxString text1 = item1->getColumnTextInternal(info->col_idx);
    wxString text2 = item2->getColumnTextInternal(info->col_idx);
    
    if (text1 == text2)
        return 0;
        
    if (info->ascending)
    {
        return (text1 < text2 ? -1 : 1);
    }
     else
    {
        return (text2 < text1 ? -1 : 1);
    }
    
    // following code not reached
    return 0;
}


void ListView::sortItems(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    int col_idx = 0;
    bool ascending = true;
    
    if (env->getParamCount() >= 1)
        col_idx = env->getParam(0)->getInteger();
    if (env->getParamCount() >= 2)
        ascending = env->getParam(1)->getBoolean();
    
    SortHelperInfo info;
    info.col_idx = col_idx;
    info.ascending = ascending;
    
    m_ctrl->SortItems(sortHelper, (long)&info);
}


void ListView::onEvent(wxEvent& evt)
{
    wxEventType event_type = evt.GetEventType();
    
    if (event_type == wxEVT_COMMAND_LIST_KEY_DOWN)
    {
        wxListEvent& tevt = (wxListEvent&)evt;
        
        // we need to figure out how we are going to handle key codes,
        // since we do not want to have a direct pass-through to
        // wxWidgets's key codes
        kscript::Value* event_args = new kscript::Value;
        event_args->getMember(L"index")->setInteger(tevt.GetIndex());
        event_args->getMember(L"keyCode")->setInteger(tevt.GetKeyCode());
        event_args->getMember(L"altDown")->setBoolean(::wxGetKeyState(WXK_ALT) ? true : false);
        event_args->getMember(L"controlDown")->setBoolean(::wxGetKeyState(WXK_CONTROL) ? true : false);
        event_args->getMember(L"shiftDown")->setBoolean(::wxGetKeyState(WXK_SHIFT) ? true : false);
        
        invokeJsEvent(L"keyDown", event_args);
    }
     else if (event_type == wxEVT_COMMAND_LIST_ITEM_SELECTED ||
              event_type == wxEVT_COMMAND_LIST_ITEM_ACTIVATED ||
              event_type == wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK ||
              event_type == wxEVT_COMMAND_LIST_BEGIN_LABEL_EDIT ||
              event_type == wxEVT_COMMAND_LIST_END_LABEL_EDIT)
    {
        wxListEvent& tevt = (wxListEvent&)evt;
        long idx = tevt.GetIndex();
        ListViewItem* data = (ListViewItem*)m_ctrl->GetItemData(idx);
        
        if (!data)
            return;
            
        kscript::Value* event_args = new kscript::Value;
        event_args->getMember(L"item")->setObject(data);
        event_args->getMember(L"index")->setInteger(idx);
    
        if (event_type == wxEVT_COMMAND_LIST_ITEM_SELECTED)
            invokeJsEvent(L"itemSelect", event_args);
         else if (event_type == wxEVT_COMMAND_LIST_ITEM_ACTIVATED)
            invokeJsEvent(L"itemActivate", event_args);
         else if (event_type == wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK)
            invokeJsEvent(L"itemRightClick", event_args);
         else if (event_type == wxEVT_COMMAND_LIST_BEGIN_LABEL_EDIT)
        {
            if (!m_allow_label_edit)
            {
                tevt.Veto();
                return;
            }
            
            invokeJsEvent(L"itemBeginLabelEdit", event_args);
        }
         else if (event_type == wxEVT_COMMAND_LIST_END_LABEL_EDIT)
        {
            data->setTextInternal(tevt.GetText());
        
            invokeJsEvent(L"itemEndLabelEdit", event_args);
        }
    }
     else if (event_type == wxEVT_COMMAND_LIST_COL_CLICK ||
              event_type == wxEVT_COMMAND_LIST_COL_RIGHT_CLICK)
    {
        // column events
        
        wxListEvent& tevt = (wxListEvent&)evt;
        long idx = tevt.GetColumn();
            
        kscript::Value* event_args = new kscript::Value;
        event_args->getMember(L"index")->setInteger(idx);

        if (event_type == wxEVT_COMMAND_LIST_COL_CLICK)
            invokeJsEvent(L"columnClick", event_args);
             else if (event_type == wxEVT_COMMAND_LIST_COL_RIGHT_CLICK)
            invokeJsEvent(L"columnRightClick", event_args);
    }
}
