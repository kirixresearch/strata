/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-09-22
 *
 */


#ifndef __XDCOMMON_DBFUNCS_H
#define __XDCOMMON_DBFUNCS_H


int xdcmnInsert(tango::IIteratorPtr sp_source_iter,
                tango::ISetPtr dest_set,
                const std::wstring& filter,
                int max_rows,
                tango::IJob* job);


bool physStructureEqual(tango::IStructurePtr s1,
                        tango::IStructurePtr s2);


#endif

