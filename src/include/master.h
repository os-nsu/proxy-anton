/*!
    \file master.h
    \brief master process life cycle
    \version 1.0
    \date 18 Jul 2024

    This file contains main loop interface and callback functions
    for work with life cycle of master process.

    IDENTIFICATION
        src/includee/master.h
*/


#ifndef MASTER_H
#define MASTER_H

/*!
    \brief Master entry point
    \return 0 if success, -1 else
*/
int mainMasterLoop (int argc, char **argv);

typedef void(*MainLoopHook)(void);

extern MainLoopHook startMainLoopHook;

extern MainLoopHook endMainLoopHook;

#endif    // MASTER_H