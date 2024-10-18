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
#include "../../include/main.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>


/*!
    \brief Loads plugins
    \param[in] argc arguments count
    \param[in] argv arguments 
    \param[out] params parsed arguments
    \return 0 if success, -1 else
*/
int parseArgs(int argc, char *argv[], Parameters *params) {
    params->flags = 0;
    params->configPath = NULL;
    params->logPath = NULL;
    params->workDirPath = NULL;
    params->pluginsPath = NULL;
    const struct option longOpts[] = {
        {"help", no_argument, NULL, 'h'},
        {"config", required_argument, NULL, 'c'},
        {"logs", required_argument, NULL, 'l'},
        {"dir", required_argument, NULL, 'D'},
        {"plugins",required_argument, NULL, 'p'},
        {NULL,0,0,0}
    };
    const char * shortOpts = "hclDp";
    int curOpt;
    while ((curOpt = getopt_long(argc, argv, shortOpts, longOpts, NULL)) != -1) {
        switch (curOpt) {
            case ('h'): {
                params->flags |= HELP_COMMAND;
                break;
            } 
            case ('c'): {
                params->configPath = optarg;
                break;
            }
            case ('l'): {
                params->logPath = optarg;
                break;
            }   
            case ('D'): {
                params->workDirPath = optarg;
                break;
            }
            case ('p'): {
                params->pluginsPath = optarg;
                break;
            }
            default: {
                return -1;
            }
        }
    }
    return 0;
}

/*!
    \brief Prints help note about program
*/
void printHelp(void) {
    printf("This is a proxy program!\n");
    printf("You can use next flags:\n");
    printf("-h (--help) shows help ;)\n");
    printf("-c (--config) defines path to directory where is file proxy.conf\n");
    printf("-l (--logs) defines path to directory where will be file proxy.log\n");
    printf("-D (--dir) defines path to working directory\n");
    printf("-p (--plugins) defines path to directory where are plugins .so files\n");
}

/*!
    \brief Main entry point in program
*/
int main(int argc, char **argv) {
    Parameters params = {0, NULL, NULL, NULL, NULL};
    int parseCode = parseArgs(argc, argv, &params);

    if (parseCode == -1) {
        return -1;
    }

    char **args = (char**)calloc(5, sizeof(char *));
    args[0] = argv[0];
    args[1] = (char *)params.workDirPath;
    args[2] = (char *)params.pluginsPath;
    args[3] = (char *)params.logPath;
    args[4] = (char *)params.configPath;

    char *allocated = NULL;
    if (!args[2]) {
        args[2] = (char *) malloc(sizeof(char) * (strlen(args[0]) - strlen("proxy") + strlen("plugins/") + 1));
        strcpy(args[2], args[0]);
        strcpy(args[2] + strlen(args[0]) - strlen("proxy"), "plugins/");
        allocated = args[2];
    }
    if (params.flags & HELP_COMMAND) {
        printHelp();
    } else {
        mainMasterLoop(5, args);
    }
    free(args);
    free(allocated);
    return 0;
}
