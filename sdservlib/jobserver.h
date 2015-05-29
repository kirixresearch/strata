/*!
 *
 * Copyright (c) 2015, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Server
 * Author:   Benjamin I. Williams
 * Created:  2015-05-26
 *
 */

 
#ifndef __SDSERVLIB_JOBSERVER_H
#define __SDSERVLIB_JOBSERVER_H



class JobServer
{
public:

    JobServer(Sdserv* sdserv)
    {
        m_sdserv = sdserv;
    }

    bool run();

public:

    Sdserv* m_sdserv;

};


#endif
