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
#include <unistd.h>
#include <stdlib.h>

/*STRUCTURES ADVERTISEMENTS*/
typedef struct InitInfoData InitInfo;

/*FUNCTIONS ADVERTISEMENTS*/
struct PluginsStack *initPluginsStack(int bootSize);
int freePlugins(struct PluginsStack *stack);
int pushPlugin(struct PluginsStack *stack, void *plugin, char *name);
struct Plugin popPlugin(struct PluginsStack *stack);
struct Plugin getPlugin(struct PluginsStack *stack, char *name);
char *mkLogPath(const char *mainDir);
char *mkConfigPath(const char *mainDir);
char *mkPluginPath(const char *fileName, const char *pluginsDir);
int forkWorkers(InitInfo *meta);
int killWorkers(void);
int loadPlugins(CATFollower *libsList, char *pluginsDir, struct PluginsStack *stack);
int launch(InitInfo *initInfo);
int exitMainLoop (InitInfo *initInfo);
int mainMasterLoop (int argc, char **argv);



/*HOOKS DEFINITIONS*/
Hook startMainLoopHook = NULL;
Hook endMainLoopHook = NULL;
Hook sharedMemoryRequestHook = NULL;
SharedMemoryHook sharedMemoryStartUpHook = NULL;




/*!
    \brief Initializes stack for plugins
    \param[in] bootSize Start size of stack
    \return Pointer to new stack 
*/
struct PluginsStack *initPluginsStack(int bootSize) {
    struct PluginsStack *stack = (struct PluginsStack *)malloc(sizeof(struct PluginsStack));
    stack->maxIdx = -1;
    stack->size = 0;
    stack->plugins = (struct Plugin *)calloc(bootSize, sizeof(struct Plugin));
    return stack;
}

/*!
    \brief Free whole elements in stack
    \param[in] stack Pointer to stack
    \return 0 if success, -1 and sets -1 else;
*/
int freePlugins(struct PluginsStack *stack) {
    for (int i = stack->maxIdx; i >= 0; i--) {
        dlclose(stack->plugins[i].handle);
    }
    return 0;
}

/*!
    \brief Pushes plugin to stack
    \param[in] stack Pointer to stack
    \param[in] plugin Plugin's pointer
    \param[in] name Plugin's name
    \return 0 if success, -1 else
*/
int pushPlugin(struct PluginsStack *stack, void *plugin, char *name) {
    if (stack->maxIdx == stack->size - 1){
        stack->size = (int)(stack->size * 1.1 + 1);
        stack->plugins = (struct Plugin *)realloc(stack->plugins,  stack->size * sizeof(struct Plugin));
    }
    stack->plugins[++stack->maxIdx].handle = plugin;
    stack->plugins[stack->maxIdx].name = name;

    return 0;
}

/*!
    \brief pop plugin from stack
    Pops plugin from stack
    \param[in] stack Pointer to stack
    \return plugin if success, NULL else
*/
struct Plugin popPlugin(struct PluginsStack *stack) {
    struct Plugin result;
    result.handle = NULL;
    result.name = NULL;
    if (stack->size == 0) {
        return result;
    }
    if (!stack->plugins[stack->maxIdx].handle) {
        return result;
    }
    return stack->plugins[stack->maxIdx--];
}

/*!
    \brief gets plugin by it's name
    \param[in] stack Pointer to plugin stack
    \param[in] name Name of plugin
    \return plugin if success, empty plugin structure else
*/
struct Plugin getPlugin(struct PluginsStack *stack, char *name) {
    struct Plugin result;
    result.handle = NULL;
    result.name = NULL;
    for(int i = 0; i < stack->size; i++) {
        if (strcmp(name, stack->plugins[i].name) == 0) {
            return stack->plugins[i];
        }
    }
    return result;
}


/*!
    \struct InitInfoData
    \brief Contains data needed for initialize
*/
struct InitInfoData {
    char *executablePath;
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
        pluginsDir = "../plugins/";
    }

    char *slash = "";
    if(pluginsDir[strlen(pluginsDir)-1] != '/') {
        slash = "/";
    }

    char *pluginPath = (char *)calloc(strlen(pluginsDir) + strlen(slash) + strlen(fileName) + strlen(".so") + 1, sizeof(char));
    return strcat(strcat(strcat(strcat(pluginPath, pluginsDir), slash),fileName), ".so"); 
}


/*!
    \brief Forks worker processes
    Forks all processes from list and execute their main fuctions.
    \param[in] meta Meta informatin for init
    \return 0 if parent process, 1 if child process, -1 if error
*/
int forkWorkers(InitInfo *meta) {
    if (!meta) {
        return -1;
    }

    int pid = 0;
    void(*bgMain)(int, void **);
    BackgroundWorker *currentWorker = backgroundWorkers.head;
    while (currentWorker) {
        /*CHECK PROCESS AFTER FORK*/
        pid = initializeBGWorker(currentWorker, meta->plugins, &bgMain);
        if (!pid) {
            if (strcmp(currentWorker->libName, "cache") == 0) {
                bgMain(1, (void **)&(meta->mainDir));
            }
            bgMain(0, NULL);
            return 1;
        } else if (pid == -1) {
            return -1;
        }
        currentWorker = currentWorker->next;
    }
    return 0;
}

/*!
    \brief Try to kill all workers
    \return 0 if success, -1 else
*/
int killWorkers(void) {
    BackgroundWorker *currentWorker = backgroundWorkers.head;
    while (currentWorker) {
        if (terminateBGWorker(currentWorker) == -1) {
            return -1;
        }
        currentWorker = currentWorker->next;
    }
    return 0;
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

    for (int i = 0; (i < *libsList->size); ++i) {
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
        pushPlugin(stack, handle, libsList->data[i].strf);

        /*EXECUTE PLUGIN*/
        initPlugin = (void(*)(void))dlsym(handle, "init");

        error = dlerror();
        if(error != NULL){
            logReport(LOG_ERROR, "Library couldn't execute init", \
                    "Library's name is %s. Dlsym message: %s", \
                    "check plugins folder or rename library", libsList->data[i].strf, error);
            return -1;
        }

        initPlugin();
    }
    return 0;
}

/*!
    \brief Initializes proxy
    Initializes logger and config, loads user's plugins, maps shared memory.
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

    /*INIT CAT & PARSE CONFIG*/

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

    /*REQUEST SHARED MEMORY*/
    if (sharedMemoryRequestHook) {
        sharedMemoryRequestHook();
    }

    /*START UP SHARED MEMORY*/
    SharedAreaManager *shMemMgr =  mapSharedMemory();
    RegionTable *table =  initRegionTable(100);
    if (sharedMemoryStartUpHook) {
        sharedMemoryStartUpHook(shMemMgr,table);
    }
    

    return 0;
}


int exitMainLoop (InitInfo *initInfo) {
    killWorkers();
    closeLogSession();
    freePlugins(initInfo->plugins);
    return 0;
}






/*!
    \brief Master entry point
    \return 0 if success, -1 else
*/
int mainMasterLoop (int argc, char **argv) {
    if (argc < 4) {
        return -1;
    }
    initWorkersList();
    char *mainDir = ".";
    if (argv[1]) {
        mainDir = argv[1];
    }

    InitInfo meta = {argv[0], mainDir, argv[2], argv[3], argv[4], initPluginsStack(100)};
    launch(&meta);

    /*FORK PROCESSES*/
    int forkRes = forkWorkers(&meta);
    if (forkRes == 1) { // if background process finished it's main function, then return
        return 0;
    }

    /*CALL HOOK*/
    if(startMainLoopHook) {
        startMainLoopHook();
    }
    printf("Hi start sleep\n");
    sleep(40);
    printf("Exit, close all workers\n");

    if(endMainLoopHook) {
        startMainLoopHook();
    }
    exitMainLoop(&meta);
    return 0;
}