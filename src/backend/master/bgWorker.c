/*!
    \file bgWorker.c
    \brief background worker
    \version 1.0
    \date 30 Jul 2024

    This file contains implementation of background worker.
    Background worker allows modules to fork their own processes.

    IDENTIFICATION
        src/background/master/bgWorker.c
*/

#include "../../include/master.h"
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>

WorkersList backgroundWorkers;

/*FUNCTION ADVERTISEMENT*/

/*!
    \brief Initializes list of workers
*/
void initWorkersList(void);

/*!
    \brief Adds new BGWorker in list
    \param[in] name Name of background worker
    \param[in] libName Name of library
    \param[in] funcName Name of the main function
    \return 0 if success, -1 else
*/
int registerBGWorker(const char *name, const char *libName, const char *funcName);

/*!
    \brief Initializes worker
    \param[in] worker Pointer to background worker
    \param[in] stack Pointer to stack of plugins
    \param[out] workerMain Pointer to main function of worker
    \return result of fork if success, -1 else
*/
int initializeBGWorker(BackgroundWorker *worker, struct PluginsStack *stack, void(**workerMain)(void));




/*FUNCTION IMPLEMENTATION*/


void initWorkersList(void) {
    backgroundWorkers.head = NULL;
    backgroundWorkers.size = 0;
}

int registerBGWorker(const char *name, const char *libName, const char *funcName) {
    if(!name || !libName || !funcName) {
        return -1;
    }

    BackgroundWorker *bgWorker = (BackgroundWorker *)malloc(sizeof(BackgroundWorker));
    bgWorker->name = (char *)calloc(strlen(name) + 1, sizeof(char));
    strcpy(bgWorker->name, name);
    bgWorker->libName = (char *)calloc(strlen(libName) + 1, sizeof(char));
    strcpy(bgWorker->libName, libName);
    bgWorker->funcName = (char *)calloc(strlen(funcName) + 1, sizeof(char));
    strcpy(bgWorker->funcName, funcName);

    bgWorker->pid = UNDEFINED_PID;
    bgWorker->status = ST_REGISTERED;

    bgWorker->next = backgroundWorkers.head;
    backgroundWorkers.head = bgWorker;
    backgroundWorkers.size++;
    return 0;
}


int initializeBGWorker(BackgroundWorker *worker, struct PluginsStack *stack, void(**workerMain)(void)) {
    if (!worker || !stack || !worker->funcName || !worker->libName ) {
        return -1;
    }

    struct Plugin plugin = getPlugin(stack, worker->libName);

    if (!plugin.handle) {
        return -1;
    }

    //LOAD MAIN FUNTION OF WORKER

    *workerMain = (void(*)(void))dlsym(plugin.handle, worker->funcName);
    
    return fork();
}

