/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-11-09
 *
 */


#ifndef H_APP_RELATIONDND_H
#define H_APP_RELATIONDND_H


#define RELATIONLINE_DATA_OBJECT_FORMAT wxT("application/vnd.kx.relline")


class RelationBox;
class RelationLineDataObject : public wxCustomDataObject
{
friend class RelationBox;
    
public:

    RelationLineDataObject()
            : wxCustomDataObject(wxDataFormat(RELATIONLINE_DATA_OBJECT_FORMAT))
    {
        size_t size = sizeof(uintptr_t)*2 + 1;
        unsigned char arr[sizeof(uintptr_t) * 2 + 1];
        memset(arr, 0, size);

        SetData(wxDataFormat(RELATIONLINE_DATA_OBJECT_FORMAT), size, (void*)arr);
    }

    virtual ~RelationLineDataObject()
    {
    }

    wxString getFields()
    {
        unsigned long* data = (unsigned long*)GetData();
        data++;
        data++;
        wxString result = (wxChar*)data;
        return result;
    }

private:

    void* GetData() const
    {
        return wxCustomDataObject::GetData();
    }

    void setInfo(RelationBox* box,
                 long line,
                 const wxString& fields)
    {
        int strdata_len = (sizeof(wxChar) * (fields.Length()+1));
        int len = (sizeof(uintptr_t)*2) + strdata_len;

        unsigned char* data = new unsigned char[len];
        uintptr_t* l = (uintptr_t*)data;

        l[0] = (uintptr_t)box;
        l[1] = (uintptr_t)line;

        memcpy(data+(sizeof(uintptr_t)*2), (const wxChar*)fields.c_str(), strdata_len);
        SetData(wxDataFormat(RELATIONLINE_DATA_OBJECT_FORMAT), len, data);

        delete[] data;
    }

    RelationBox* getBox()
    {
        uintptr_t* data = (uintptr_t*)GetData();
        return (RelationBox*)data[0];
    }

    long getLine()
    {
        uintptr_t* data = (uintptr_t*)GetData();
        return (long)data[1];
    }
};




class RelationLineDropTarget : public wxDropTarget
{
public:
        
    RelationLineDropTarget() : wxDropTarget(new RelationLineDataObject)
    {
    }

    wxDragResult OnEnter(wxCoord x,
                         wxCoord y,
                         wxDragResult def)
    {
        sigDragEnter(def);
        return def;
    }

    void OnLeave()
    {
        sigDragLeave();
    }

    wxDragResult OnDragOver(wxCoord x,
                            wxCoord y,
                            wxDragResult def)
    {
        #ifndef WIN32
            def = wxDropTarget::OnDragOver(x, y, def);
        #endif

        sigDragOver(def);
        return def;
    }

    wxDragResult OnData(wxCoord x,
                        wxCoord y,
                        wxDragResult def)
    {
        if (!GetData())
            return wxDragNone;

        RelationLineDataObject* data = (RelationLineDataObject*)GetDataObject();
        if (!data)
            return wxDragNone;

        sigDragDrop.fire(def, data);

        return def;
    }

public: // signals

    xcm::signal1<wxDragResult&> sigDragEnter;
    xcm::signal1<wxDragResult&> sigDragOver;
    xcm::signal0 sigDragLeave;
    xcm::signal2<wxDragResult&, RelationLineDataObject*> sigDragDrop;

private:

    wxDataFormat m_format;
};


#endif  // __APP_RELATIONDND_H

