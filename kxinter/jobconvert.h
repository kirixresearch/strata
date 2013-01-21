/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   Benjamin I. Williams
 * Created:  2003-05-21
 *
 */


#ifndef __KXINTER_JOBCONVERT_H
#define __KXINTER_JOBCONVERT_H


#include "datastream.h"
#include "datadef.h"


class Connection;


struct ConvertDataInstruction
{
    IDataStreamPtr m_stream;
    DataFormatDefinition m_dfd;
    tango::IDatabasePtr m_db;
    wxString m_table_name;
};


class ConvertDataJob : public cfw::JobBase
{

XCM_CLASS_NAME("kxinter.ConvertDataJob")
XCM_BEGIN_INTERFACE_MAP(ConvertDataJob)
    XCM_INTERFACE_CHAIN(cfw::JobBase)
XCM_END_INTERFACE_MAP()

public:

    ConvertDataJob();
    ~ConvertDataJob();

    void addInstruction(IDataStream* stream,
                        const DataFormatDefinition& dfd,
                        tango::IDatabasePtr destination,
                        const wxString& destination_table);

    bool convertTable(ConvertDataInstruction& cdi);
    int runJob();

private:

    unsigned char* getNextFixedRecord(int* reclen);
    unsigned char* getNextVariableRecord(int* reclen);
    unsigned char* getNextDelimitedRecord(int* reclen);
    unsigned char* getNextRecord(int* reclen);

private:

    std::vector<ConvertDataInstruction> m_instructions;

    cfw::IJobStatPtr m_skipped_stat;
    cfw::IJobStatPtr m_processed_stat;

    IDataStreamPtr m_stream;
    DataFormatDefinition m_dfd;

    unsigned char m_line_delimiter[64];
    int m_line_delimiter_length;

    int m_record_length;
    int m_record_type;
    int m_skip_bytes;
    unsigned char* m_curpos;
    unsigned char* m_buf;
    int m_buf_size;
};




class ConvertJob : public cfw::JobBase
{

XCM_CLASS_NAME("kxinter.ConvertJob")
XCM_BEGIN_INTERFACE_MAP(ConvertJob)
    XCM_INTERFACE_CHAIN(cfw::JobBase)
XCM_END_INTERFACE_MAP()

public:

    ConvertJob();
    ~ConvertJob();

    void setInstructions(tango::IDatabasePtr src_db, const wxString& src_table,
                         tango::IDatabasePtr dest_db, const wxString& dest_table,
                         cfw::ITreeItemPtr src_item, cfw::ITreeItemPtr dest_item);

    // -- this function was added temporarily so Aaron could use kxinter instead of the 
    //    sql server enterprise manager while doing some data analysis --
    void createAndSaveSetFromIterator(tango::IDatabasePtr target_db,
                                      const wxString& set_name,
                                      tango::IIteratorPtr src_iter);

    int runJob();
    void runPostJob();

private:

    tango::IDatabasePtr m_src_db;
    tango::IDatabasePtr m_dest_db;



    wxString m_src_table;
    wxString m_dest_table;

    cfw::ITreeItemPtr m_src_item;
    cfw::ITreeItemPtr m_dest_item;
};





#endif


