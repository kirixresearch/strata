/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   Benjamin I. Williams
 * Created:  2003-05-06
 *
 */


#ifndef __KXINTER_DATASTREAM_H
#define __KXINTER_DATASTREAM_H


#ifdef _MSC_VER
typedef __int64 ds_off_t;
#else
typedef long long ds_off_t;
#endif


xcm_interface IDataStream;
XCM_DECLARE_SMARTPTR(IDataStream)


xcm_interface IDataStream : public xcm::IObject
{
    XCM_INTERFACE_NAME("kxinter.IDataStream")

public:

    virtual int getData(unsigned char* buf, int read_len) = 0;
    virtual ds_off_t getPosition() = 0;
    virtual bool setPosition(const ds_off_t& offset) = 0;
    virtual ds_off_t getStreamSize() = 0;
    virtual IDataStreamPtr clone() = 0;
};



IDataStreamPtr openFileStream(const wxString& filename);



#endif




