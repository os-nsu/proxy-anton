/*!
    \file main.h
    \brief header for entry point in program
    \version 1.0
    \date Oct 18 2024

    This file contains data structures required by main.c

    IDENTIFICATION
        src/include/main.h
*/
#ifndef MAIN_H
#define MAIN_H


#define HELP_COMMAND (0x00000001)


/*!
    \struct Parameters
    \brief Plain of parameters and options
*/
typedef struct ParametersInfo {
    int flags;
    const char *workDirPath;
    const char *configPath;
    const char *logPath;
    const char *pluginsPath;
} Parameters;

#endif