/*!
 *
 * Copyright (c) 2012, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2012-04-18
 *
 */



#ifndef __XDCOMMON_GROUPQUERYENGINE_H
#define __XDCOMMON_GROUPQUERYENGINE_H


namespace xdcommon
{




bool runGroupQuery(tango::IDatabasePtr db,
                   tango::IIteratorPtr iter,
                   const std::wstring& output_path,
                   const std::wstring& group,
                   const std::wstring& output,
                   const std::wstring& condition,
                   const std::wstring& having,
                   tango::IJob* job);


}; // namespace xdcommon

#endif

