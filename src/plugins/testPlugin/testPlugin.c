#include <stdio.h>
#include "../../include/master.h"
#include "../../include/logger.h"
#include <unistd.h>

static Hook prevStartMainLoopHook = NULL;
static Hook prevSharedMemoryRequestHook = NULL;
static SharedMemoryHook prevSharedMemoryStartUpHook = NULL;

void customStartMainLoopHook(void);
void customSharedMemoryRequestHook(void);
void customSharedMemoryStartUpHook(SharedAreaManager *manager, RegionTable *table);


void init(void);
void bgMain(void);

long long *mycounter = NULL;


void init(void) {
    printf("Hello, world, from init!\n");

    prevStartMainLoopHook = startMainLoopHook;
    startMainLoopHook = customStartMainLoopHook;

    prevSharedMemoryRequestHook = sharedMemoryRequestHook;
    sharedMemoryRequestHook = customSharedMemoryRequestHook;

    prevSharedMemoryStartUpHook = sharedMemoryStartUpHook;
    sharedMemoryStartUpHook = customSharedMemoryStartUpHook;

    registerBGWorker("myWorker", "test", "bgMain");
}

void customStartMainLoopHook(void) {
    if(prevStartMainLoopHook) {
        prevStartMainLoopHook();
    }
    printf("my custom start loop hook implementation!\n");
}

void customSharedMemoryRequestHook(void){
    if (prevSharedMemoryRequestHook) {
        prevSharedMemoryRequestHook();
    }
    requestSharedMemory(sizeof(long long));
}

void customSharedMemoryStartUpHook(SharedAreaManager *manager, RegionTable *table) {
    if(prevSharedMemoryStartUpHook) {
        prevSharedMemoryStartUpHook(manager, table);
    }
    int flFound = -1;
    mycounter = registerSharedArea(manager, table, "testCounter", sizeof(long long), &flFound);
    *mycounter = 0LL;
}

void bgMain(void) {
    int i = 0;
    while(1){
        logMsg(LOG_INFO, LOG_PRIMARY, "Hello %d", i);
        i++;
        if (i == 10) {
            return;
        }
        (*mycounter)++;
        printf("in plugin %lld", *mycounter);
    }

}