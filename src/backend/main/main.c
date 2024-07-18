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



/*!
    \brief Main entry point in program
*/
int main(int argc, char **argv) {
    ///< \todo parser of command line arguments
    char **args = (char**)calloc(3, sizeof(char *));
    mainMasterLoop(3, args);
    return 0;
}

