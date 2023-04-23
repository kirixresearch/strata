/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-09-22
 *
 */


#ifndef H_XDCOMMON_DBFUNCS_H
#define H_XDCOMMON_DBFUNCS_H


int xdcmnInsert(xd::IDatabasePtr db,
                xd::IIteratorPtr sp_source_iter,
                const std::wstring& dest_table,
                const std::vector< std::pair<std::wstring, std::wstring> >& columns,
                const std::wstring& filter,
                int max_rows,
                xd::IJob* job);



#endif

