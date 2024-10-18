/*!
    \file main.c
    \brief This file is main entry point in proxy
    \version 1.0
    \date 18 Jul 2024

    This file contains methods for working with parameters.
    In main function comand line arguments are parsed. After that
    main execute mainMasterLoop

    IDENTIFICATION
        src/backend/main/main.c
*/


#include "../../include/master.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*!
    \brief Main entry point in program
*/
int main(int argc, char **argv) {
    ///< \todo parser of command line arguments
    char **args = (char**)calloc(4, sizeof(char *));
    args[0] = argv[0];
    if (argc > 2) {
        args[1] = argv[1];
        args[2] = argv[2];
    } else if (argc <= 2) {
        args[2] = (char *) malloc(sizeof(char) * (strlen(args[0]) - strlen("proxy") + strlen("plugins/") + 1));
        strcpy(args[2], args[0]);
        strcpy(args[2] + strlen(args[0]) - strlen("proxy"), "plugins/");
    }
    mainMasterLoop(4, args);
    return 0;
}