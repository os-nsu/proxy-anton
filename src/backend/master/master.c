/*!
    \file master.c
    \brief master process life cycle
    \version 1.0
    \date 18 Jul 2024

    This file contains main loop of master process. In this file
    plugins are loaded and processes fork.

    IDENTIFICATION
        src/backend/master/master.c
*/


#include "../../include/master.h"
#include "../../include/logger.h"
#include "../../include/config.h"
#include <stdio.h>
#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>

/*STRUCTURES ADVERTISEMENTS*/
struct PluginsStack;
typedef struct InitInfoData InitInfo;

/*FUNCTIONS ADVERTISEMENTS*/
struct PluginsStack *initPluginsStack(int bootSize);
int freePlugins(struct PluginsStack *stack);
int pushPlugin(struct PluginsStack *stack, void *plugin);
void *popPlugin(struct PluginsStack *stack);
char *mkLogPath(const char *mainDir);
char *mkConfigPath(const char *mainDir);
char *mkPluginPath(const char *fileName, const char *pluginsDir);
int loadPlugins(CATFollower *libsList, char *pluginsDir, struct PluginsStack *stack);
int launch(InitInfo *initInfo);
int exitMainLoop (InitInfo *initInfo);
int mainMasterLoop (int argc, char **argv);



/*HOOKS DEFINITIONS*/
MainLoopHook startMainLoopHook = NULL;

MainLoopHook endMainLoopHook = NULL;


/*!
    \struct PluginsStack
    \brief Stack for plugin pointers
*/
struct PluginsStack {
    void **plugins;
    int size;
    int maxIdx;
};


/*!
    \brief Initializes stack for plugins
    \param[in] bootSize Start size of stack
    \return Pointer to new stack 
*/
struct PluginsStack *initPluginsStack(int bootSize) {
    struct PluginsStack *stack = (struct PluginsStack *)malloc(sizeof(struct PluginsStack));
    stack->maxIdx = -1;
    stack->size = 0;
    stack->plugins = (void **)calloc(bootSize, sizeof(void *));
    return stack;
}

/*!
    \brief Free whole elements in stack
    \param[in] stack Pointer to stack
    \return 0 if success, -1 and sets -1 else;
*/
int freePlugins(struct PluginsStack *stack) {
    for (int i = stack->maxIdx; i >= 0; i--) {
        dlclose(stack->plugins[i]);
    }
    return 0;
}

/*!
    \brief Pushes plugin to stack
    \param[in] stack Pointer to stack
    \param[in] plugin Plugin's pointer
    \return 0 if success, -1 else
*/
int pushPlugin(struct PluginsStack *stack, void *plugin) {
    if (stack->maxIdx == stack->size - 1){
        stack->size = (int)(stack->size * 1.1 + 1);
        stack->plugins = (void **)realloc(stack->plugins,  stack->size * sizeof(void *));
    }
    stack->plugins[++stack->maxIdx] = plugin;

    return 0;
}

/*!
    \brief pop plugin from stack
    \param[in] stack Pointer to stack
    \return plugin if success, NULL else
*/
void *popPlugin(struct PluginsStack *stack) {
    if (!stack->plugins[stack->maxIdx]) {
        return NULL;
    }
    return stack->plugins[stack->maxIdx--];
}

/*!
    \struct InitInfoData
    \brief Contains data needed for initialize
*/
struct InitInfoData {
    char *mainDir;
    char *pluginsDir;
    char *loggerPath;
    char *configPath;
    struct PluginsStack *plugins;
};

/*!
    \brief make loggerPath using mainDir
    \param[in] mainDir path to main proxy directory
    \return loggerPath if success, NULL else
*/
char *mkLogPath(const char *mainDir) {
    if (!mainDir) {
        return NULL;
    }
    
    /*DEFINE LOG FILE NAME*/
    char *logFileName;
    const char *c = mainDir;
    while (*c++);

    if (*(c--) == '/') {
        logFileName = "proxy.log";
    } else {
        logFileName = "/proxy.log";
    }
    
    char *loggerPath = (char *)calloc(strlen(mainDir) + strlen(logFileName) + 1, sizeof(char));
    return strcat(strcat(loggerPath, mainDir), logFileName);
}

/*!
    \brief make path for config using mainDir
    \param[in] mainDir path to main proxy directory
    \return configrPath if success, NULL else
*/
char *mkConfigPath(const char *mainDir) {
    if (!mainDir) {
        return NULL;
    }
    
    /*DEFINE CONFIG FILE NAME*/
    char *configFileName;
    const char *c = mainDir;
    while (*c++);

    if (*(c--) == '/') {
        configFileName = "proxy.conf";
    } else {
        configFileName = "/proxy.conf";
    }
    
    char *configPath = (char *)calloc(strlen(mainDir) + strlen(configFileName) + 1, sizeof(char));
    return strcat(strcat(configPath, mainDir), configFileName);
}

/*!
    \brief creates path to plugin
    \param[in] fileName Name of dynamic library
    \param[in] pluginsDir Path to plugins directory (could be NULL)
    \return 0 if success, -1 else
*/
char *mkPluginPath(const char *fileName, const char *pluginsDir) {
    if (!fileName) {
        return NULL;
    }

    if (!pluginsDir){
        pluginsDir = "../../plugins/";
    }

    char *slash = "";
    if(pluginsDir[strlen(pluginsDir)-1] != '/') {
        slash = "/";
    }

    char *pluginPath = (char *)calloc(strlen(pluginsDir) + strlen(slash) + strlen(fileName) + strlen(".so") + 1, sizeof(char));
    return strcat(strcat(strcat(strcat(pluginPath, pluginsDir), slash),fileName), ".so"); 
}

/*!
    \brief Loads plugins
    \param[in] libsList Follower to parameter "plugins"
    \param[in] pluginsDir plugins directory (may be NULL) 
    \param[in] stack Stack for plugins
    \return 0 if success, -1 else
*/
int loadPlugins(CATFollower *libsList, char *pluginsDir, struct PluginsStack *stack) {
    
    void (*initPlugin)(void); ///< this function will be executed for each contrib library
    void *handle;
    char *error;

    for (int i = 0; i < *libsList->size; ++i) {
        /*TRY TO LOAD .SO FILE*/
        char* pluginPath = mkPluginPath(libsList->data[i].strf, pluginsDir);
        handle = dlopen(pluginPath, RTLD_NOW | RTLD_GLOBAL);

        if (!handle) {
            error = dlerror();
            logReport(LOG_ERROR, "Library couldn't be opened", \
                "Library's path is %s\n dlopen: %s\n", "check plugins folder or rename library", pluginPath, error);
            return -1;
        }
        error = dlerror();

        /*PUSH HANDLE TO PLUGIN STACK*/

        pushPlugin(stack, handle);

        /*EXECUTE PLUGIN*/
        initPlugin = (void(*)(void))dlsym(handle, "init");

        error = dlerror();
        if(error != NULL){
            logReport(LOG_ERROR, "Library couldn't execute init", \
                    "Library's name is %s. Dlopen message: %s", \
                    "check plugins folder or rename library", libsList->data[i].strf, error);
            return -1;
        }

        initPlugin();
    }
    return 0;
}

/*!
    \brief Initializes proxy
    Initializes logger and config, loads user's plugins, fork other processes.
    \param[in] paths WorkPaths structure that contains need paths
    \return 0 if success, -1 and sets errno else
*/
int launch(InitInfo *initInfo) {
    
    /*Check and set paths*/
    if (!initInfo->mainDir) {
        return -1;
    }
    
    if (!initInfo->loggerPath) {
        initInfo->loggerPath = mkLogPath(initInfo->mainDir);

        if (!initInfo->loggerPath) {
            return -1;
        }
    }
    
    if (!initInfo->configPath) {
        initInfo->configPath = mkConfigPath(initInfo->mainDir);

        if (!initInfo->configPath) {
            return -1;
        }
    }
    
    /*OPEN LOGGER*/
    if (initLogger(initInfo->loggerPath)) {
        fprintf(stderr, "Logger couldn't be initialized\n");
    }
    if (openLogSession()) {
        fprintf(stderr, "Logger session couldn't be opened. Log file path is \"%s\"\n", initInfo->loggerPath);
    }

    if (initCAT()) {
        logReport(LOG_ERROR, "CAT initialize error", NULL, "problem with allocation, check memory");
    }

    if(parseConfig(initInfo->configPath)) {
        return -1;
    }
    
    /*Read plugin list*/
    CATFollower contribLibs;
    if( addFollowerToCAT("kernel", "plugins", &contribLibs)) {
   
        logReport(LOG_WARNING, "proxy does not see any plugins in init list",\
        "plugins set in config, config path is %s","check config file: key = \"plugins\"", initInfo->configPath);
    } else {
        if(loadPlugins(&contribLibs, initInfo->pluginsDir, initInfo->plugins)) {
            return -1;
        }
    }
    
    return 0;
}

int exitMainLoop (InitInfo *initInfo) {

    closeLogSession();
    freePlugins(initInfo->plugins);
    return 0;
}

/*!
    \brief Master entry point
    \return 0 if success, -1 else
*/
int mainMasterLoop (int argc, char **argv) {
    if (argc < 3) {
        return -1;
    }
    InitInfo meta = {".", argv[0], argv[1], argv[2], initPluginsStack(100)};
    launch(&meta);
    
    /*CALL HOOK*/
    printf("start hook is %p\nend hook is %p\n", startMainLoopHook, endMainLoopHook);
    if(startMainLoopHook) {
        startMainLoopHook();
    }
    

    if(endMainLoopHook) {
        startMainLoopHook();
    }
    exitMainLoop(&meta);
    return 0;
}


