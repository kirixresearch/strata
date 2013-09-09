/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-03-27
 *
 */


#include "scripthost.h"
#include "system.h"
#include <kl/thread.h>
#include <ctime>

#ifdef WIN32
#include <windows.h>
#endif


// (CLASS) System
// Category: System
// Description: A class that provides system utility functions.
// Remarks: The System class provides basic system functionality, such as the
//     sleep() function for waiting, or the clock() function for timing functionality.

System::System()
{
}

System::~System()
{
}


void System::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}


// (METHOD) System.beep
//
// Description: Creates a system beep.
//
// Syntax: static function System.beep(frequency : Integer, 
//                                     duration : Integer)
//
// Remarks: Creates a system beep.  If no parameters are specified,
//     the function creates a default system beep.  If parameters
//     are specified, the function creates a beep having the given
//     |frequency| and |duration|.
//
// Param(frequency): The |frequency| of the beep in hertz.
// Param(duration): The |duration| of the beep in milliseconds.
//

void System::beep(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
#ifdef WIN32
    if (env->getParamCount() < 2)
    {
        ::MessageBeep((UINT)-1);
    }
    else
    {
        int frequency = env->getParam(0)->getInteger();
        int duration = env->getParam(1)->getInteger();
        ::Beep(frequency, duration);
    }
#else
// TODO: implement for other platforms
#endif
}

// (METHOD) System.time
//
// Description: Returns the number of seconds elapsed since January 1, 1970 UTC.
//
// Syntax: static function System.time() : Number
//
// Remarks: Returns the number of seconds elapsed since 00:00 hours,
//     January 1, 1970 UTC.
//
// Returns: The number of seconds elapsed since January 1, 1970 UTC.

void System::ztime(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    time_t t = time(NULL);
    retval->setDouble((double)t);
}

// (METHOD) System.clock
//
// Description: Returns the CPU clock value.
//
// Syntax: static function System.clock() : Number
//
// Remarks: Returns the CPU clock value.  This is useful for measuring how long a
//     piece of code takes to run.  It is a basic tool for performance testing.
//     By calling the getClocksPerSecond() function, the user can determine how many
//     seconds have elapsed, to a high degree of accuracy.
//
// Returns: The value of CPU clock.

void System::clock(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    clock_t c = ::clock();
    retval->setDouble((double)c);
}



// (METHOD) System.getClocksPerSecond
//
// Description: Returns number of CPU clock ticks per second.
//
// Syntax: static function System.getClocksPerSecond() : Number
//
// Remarks: Returns number of CPU clock ticks per second.  This is useful in
//      conjunction with the clock() method.
//
// Returns: The number of clock ticks per second.

void System::getClocksPerSecond(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    retval->setDouble((double)CLOCKS_PER_SEC);
}



// (METHOD) System.sleep
//
// Description: Pauses execution for a specified number of milliseconds
//
// Syntax: static function System.sleep(milliseconds : Number)
//
// Remarks: Pauses execution in a thread-friendly manner for the specified number
//     of milliseconds.
//
// Param(milliseconds): The number of milliseconds to sleep.

void System::sleep(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    if (env->getParamCount() > 0)
    {
        int amt = env->getParam(0)->getInteger();
        
        while (1)
        {
            if (amt <= 0)
                return;
                
            if (amt < 1000)
            {
                kl::Thread::sleep(amt);
                amt = 0;
            }
             else
            {
                kl::Thread::sleep(1000);
                amt -= 1000;
            }
            
            if (env->isCancelled())
                return;
        }
        
    }
}



// (METHOD) System.execute
//
// Description: Execute a system command; Run a program
//
// Syntax: static function System.execute(command : String) : Integer
//
// Remarks: Calling execute runs another program.  The program is
//     launched asynchronously, meaning that execution in the calling
//     scope is not blocked, and the program will be run in the background.
//     The |command| parameter represents the entire command line, including
//     program path and command line parameters.
//
// Param(command): The command line to execute, including the program path
//     and command line parameters.
// Returns: The process id of the new process, or zero if the operation failed.

void System::execute(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setInteger(0);
        return;
    }
    
    /*
    retval->setInteger(::wxExecute(env->getParam(0)->getString()));
    */
    // TODO: reimplement
}
