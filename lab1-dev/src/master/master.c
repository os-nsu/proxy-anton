#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *mkPuginPath(char* execPath, const char *pluginRelPath) {
    char * resultPath = (char *)malloc(sizeof(char) * (strlen(execPath) - 4 + strlen(pluginRelPath)));
    strcpy(resultPath, execPath);
    resultPath[strlen(execPath)-5] = 0;
    strcat(resultPath, pluginRelPath);
    return resultPath;
}

int main(int argc, char *argv[]) {
    void (*initPlugin)(void); ///< this function will be executed for each contrib library
    void *handle;
    char *error;
    /*TRY TO LOAD .SO FILE*/
    const char * pluginRelPath = "../contrib/plugin.so";
    char * pluginPath = mkPuginPath(argv[0], pluginRelPath);     
    handle = dlopen(pluginPath, RTLD_NOW | RTLD_GLOBAL);

    if (!handle) {
        error = dlerror();
        fprintf(stderr, "Library couldn't be opened\nLibrary's path is %s\n dlopen: %s\ncheck plugins folder or rename library", pluginPath, error);
        return -1;
    }
    error = dlerror();

    /*EXECUTE PLUGIN*/
    initPlugin = (void(*)(void))dlsym(handle, "init");

    error = dlerror();
    if(error != NULL){
        fprintf(stderr, "Library couldn't execute init\nLibrary's name is %s. Dlsym message: %s\ncheck plugins folder or rename library", pluginPath, error);
        return -1;
    }

    initPlugin();


    free(pluginPath);

    return 0;
}