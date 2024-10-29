/*!
    \file main.c
    \brief This file is main entry point in proxy
    \version 1.0
    \date 24 Oct 2024

    This file contains methods for working with parameters.
    In main function comand line arguments are parsed. After that
    main execute mainMasterLoop

    IDENTIFICATION
        src/backend/main/main.c
*/

#include "../../include/master.h"
#include "../../include/config.h"
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

    /*INIT CAT*/

    if (initCAT()) {
        fprintf(stderr, "CAT initialize error\n\tproblem with allocation, check memory");
    }

    /*SET PARAMETERS FROM COMMAND LINE*/

    char *executablePath = (char *)calloc(strlen(argv[0]) + 1, sizeof(char));
    strcpy(executablePath, argv[0]);

    createCATParameter("kernel", "executablePath", T_STRING, 1, (union Value *)(&executablePath), NULL, "path to executable");
    createCATParameter("kernel", "pluginsDir", T_STRING, 1, (union Value *)(&params.pluginsPath), NULL, "path to directory with plugins");
    createCATParameter("kernel", "configPath", T_STRING, 1, (union Value *)(&params.configPath), NULL, "path to config");
    createCATParameter("kernel", "dataDir", T_STRING, 1, (union Value *)(&params.workDirPath), NULL, "path to data directory");
    createCATParameter("kernel", "logsDir", T_STRING, 1, (union Value *)(&params.logPath), NULL, "path to directory with log files");  
    
    if (params.flags & HELP_COMMAND) {
        printHelp();
    } else {
        mainMasterLoop();
    }
    free(executablePath);
    return 0;
}
