/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Server
 * Author:   Benjamin I. Williams
 * Created:  2014-10-22
 *
 */


#ifndef __SDSERVLIB_WEBSERV_H
#define __SDSERVLIB_WEBSERV_H

#ifdef __cplusplus
extern "C" {
#endif // 


struct wsrv_context
{
    char* config[60];  // config options
};



struct wsrv_context* wsrv_start();




#ifdef __cplusplus
}
#endif



#endif
