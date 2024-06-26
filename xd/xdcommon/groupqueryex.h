/*!
 *
 * Copyright (c) 2012-2013, Kirix Research, LLC.  All rights reserved.
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




bool runGroupQuery(xd::IDatabasePtr db,
                   xd::IIteratorPtr iter,
                   const std::wstring& output_path,
                   const std::wstring& group,
                   const std::wstring& output,
                   const std::wstring& condition,
                   const std::wstring& having,
                   xd::IJob* job);


}; // namespace xdcommon

#endif

