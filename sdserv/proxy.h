/*!
 *
 * Copyright (c) 2012-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Proxy
 * Author:   Benjamin I. Williams
 * Created:  2012-07-30
 *
 */


#include "sdserv.h"
#include "mongoose.h"
#include "request.h"
#include "controller.h"


class Proxy
{
public:
    
    Proxy();
    ~Proxy();
    
    int runServer();
};
