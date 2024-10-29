//test pugin source code for the first lab
#include "../../src/include/master.h"
#include <stdio.h>


static Hook prev_start_hook = NULL;
static Hook prev_end_hook = NULL;

void custom_start_hook(void);
void custom_end_hook(void);

void init(void);


void init(void) {
    prev_start_hook = start_hook;
    prev_end_hook = end_hook;
    start_hook = custom_start_hook;
    end_hook = custom_end_hook;
    
    printf("init successfully\n");
}


void custom_start_hook(void) {
    if (prev_start_hook) {
        prev_start_hook();
    }
    printf("hello from custom_start_hook()\n");
}


void custom_end_hook(void) {
    if (prev_end_hook) {
        prev_end_hook();
    }
    printf("hello from custom_end_hook()\n");
}