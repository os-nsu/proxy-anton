#include <stdio.h>
#include <stdlib.h>

int main(void) {

    char* name = (char*)malloc(30);
    scanf("%s\n",name);
    printf("Hello %s!\n", name);

    return 0;
}