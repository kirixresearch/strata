/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-09-22
 *
 */


#ifndef __XDCOMMON_DBFUNCS_H
#define __XDCOMMON_DBFUNCS_H


int xdcmnInsert(tango::IDatabasePtr db,
                tango::IIteratorPtr sp_source_iter,
                const std::wstring& dest_table,
                const std::wstring& filter,
                int max_rows,
                tango::IJob* job);


bool physStructureEqual(tango::IStructurePtr s1,
                        tango::IStructurePtr s2);


#endif

