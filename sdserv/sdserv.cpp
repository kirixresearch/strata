/*!
 *
 * Copyright (c) 2012-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Server
 * Author:   Benjamin I. Williams
 * Created:  2012-07-30
 *
 */


#include "sdserv.h"
#include "../sdservlib/sdservlib.h"
#include <kl/file.h>



int main(int argc, const char** argv)
{
    Sdserv sdserv;

    if (!sdserv.initOptionsFromCommandLine(argc, argv))
    {
        sdserv.signalServerNotReady();
        return 0;
    }


    sdserv.runServer();

    return 0;
}

