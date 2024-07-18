#include <stdio.h>
#include "../include/master.h"

static MainLoopHook prevStartMainLoopHook = NULL;

void customStartMainLoopHook(void);
void init(void);
void init(void) {
    printf("Hello, world, from init!\n");

    prevStartMainLoopHook = startMainLoopHook;
    startMainLoopHook = customStartMainLoopHook;
    printf("new start impl %p\n", startMainLoopHook);
}

void customStartMainLoopHook(void) {
    if(prevStartMainLoopHook) {
        prevStartMainLoopHook();
    }
    printf("my custom hook implementation!\n");
}