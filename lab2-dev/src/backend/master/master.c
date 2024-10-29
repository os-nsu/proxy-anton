/*!
    \file master.c
    \brief master process life cycle
    \version 1.0
    \date 24 Oct 2024

    This file contains main loop of master process. In this file
    plugins are loaded and processes fork.

    IDENTIFICATION
        src/backend/master/master.c
*/


#include "../../include/master.h"
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
int loadPlugins(CATFollower *libsList, char *pluginsDir, struct PluginsStack *stack);
int launch(InitInfo *initInfo);
int exitMainLoop (InitInfo *initInfo);
int mainMasterLoop (void);



/*HOOKS DEFINITIONS*/
Hook start_hook = NULL;
Hook end_hook = NULL;




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
    while (*c) {
        c++;
    }
    
    if (*(--c) == '/') {
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
    while (*(c)) {
        c++;
    }

    if (*(--c) == '/') {
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
            fprintf(stderr, "Library couldn't be opened.\n\tLibrary's path is %s\n\tdlopen: %s\n\tcheck plugins folder or rename library",  pluginPath, error);
            return -1;
        }
        error = dlerror();

        /*PUSH HANDLE TO PLUGIN STACK*/
        pushPlugin(stack, handle, libsList->data[i].strf);

        /*EXECUTE PLUGIN*/
        initPlugin = (void(*)(void))dlsym(handle, "init");

        error = dlerror();
        if(error != NULL){
            fprintf(stderr, "Library couldn't execute init.\n\tLibrary's name is %s. Dlsym message: %s\n\tcheck plugins folder or rename library", libsList->data[i].strf, error);
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
    
    CATFollower dataDirFlwr, configPathFlwr, pluginsDirFlwr;
    if(addFollowerToCAT("kernel", "dataDir", &dataDirFlwr) || !dataDirFlwr.data->strf) {
        return -1;
    }

    if(addFollowerToCAT("kernel", "pluginsDir", &pluginsDirFlwr) || !pluginsDirFlwr.data->strf) {
        return -1;
    }

    if(addFollowerToCAT("kernel", "configPath", &configPathFlwr)) {
        char *configPath = mkConfigPath(dataDirFlwr.data->strf);
        if (!configPath) {
            return -1;
        }
        createCATParameter("kernel", "configPath", T_STRING, 1, (union Value *)(&configPath), &configPathFlwr, "path to config");
        free(configPath);
    } else if (!configPathFlwr.data->strf) {
        char *configPath = mkConfigPath(dataDirFlwr.data->strf);
        
        if (!configPath) {
            return -1;
        }
        int res = updateCATParameter("kernel", "configPath", 1, (union Value *)(&configPath));
        free(configPath);
    }


    //PARSE CONFIG

    if(parseConfig(configPathFlwr.data->strf)) {
        return -1;
    }
    
    /*Read plugin list*/
    CATFollower contribLibs;
    if( addFollowerToCAT("kernel", "plugins", &contribLibs)) {
        fprintf(stderr, "proxy does not see any plugins in init list\n\tplugins set in config, \
        config path is %s\n\tcheck config file: key = \"plugins\"", configPathFlwr.data->strf);
    } else {
        if(loadPlugins(&contribLibs, pluginsDirFlwr.data->strf, initInfo->plugins)) {
            return -1;
        }
    }

    removeFollowerFromCAT("kernel", "dataDir", &dataDirFlwr);
    removeFollowerFromCAT("kernel", "pluginsDir", &pluginsDirFlwr);
    removeFollowerFromCAT("kernel", "configPath", &configPathFlwr);
    removeFollowerFromCAT("kernel", "plugins", &contribLibs);
    return 0;
}


int exitMainLoop (InitInfo *initInfo) {
    freePlugins(initInfo->plugins);
    return 0;
}


/*!
    \brief Master entry point
    \return 0 if success, -1 else
*/
int mainMasterLoop (void) {
    CATFollower dataDirFlwr, executablePathFlwr, configPathFlwr, logsDirFlwr, pluginsDirFlwr;
    if (addFollowerToCAT("kernel", "dataDir", &dataDirFlwr) == -1) {
        const char *dataDir = ".";  
        createCATParameter("kernel", "dataDir", T_STRING, 1,(union Value *)(&dataDir), &dataDirFlwr, "path to data directory");
    } else if (!dataDirFlwr.data->strf) {
        const char *dataDir = ".";
        updateCATParameter("kernel", "dataDir", 1, (union Value *)(&dataDir));
    }
    
    removeFollowerFromCAT("kernel", "dataDir", &dataDirFlwr);

    if (addFollowerToCAT("kernel", "executablePath", &executablePathFlwr) == -1 ||  !executablePathFlwr.data->strf) {
        fprintf(stderr, "configuration has no path to executable");
        return -1;
    }
    
    if (addFollowerToCAT("kernel", "pluginsDir", &pluginsDirFlwr) == -1 ) {
        char *pluginsDir = (char *) malloc(sizeof(char) * (strlen(executablePathFlwr.data->strf) - strlen("proxy") + strlen("plugins/") + 1));
        strcpy(pluginsDir, executablePathFlwr.data->strf);
        strcpy(pluginsDir + strlen(executablePathFlwr.data->strf) - strlen("proxy"), "plugins/");
        createCATParameter("kernel", "pluginsDir", T_STRING, 1,(union Value *)(&pluginsDir), &pluginsDirFlwr, "path to directory with plugins");
        free(pluginsDir);
    } else if (!pluginsDirFlwr.data->strf) {
        char *pluginsDir = (char *) malloc(sizeof(char) * (strlen(executablePathFlwr.data->strf) - strlen("proxy") + strlen("plugins/") + 1));
        strcpy(pluginsDir, executablePathFlwr.data->strf);
        strcpy(pluginsDir + strlen(executablePathFlwr.data->strf) - strlen("proxy"), "plugins/");
        updateCATParameter("kernel", "pluginsDir", 1,(union Value *)(&pluginsDir));
        free(pluginsDir);
    }

    removeFollowerFromCAT("kernel", "executablePath", &executablePathFlwr);
    removeFollowerFromCAT("kernel", "pluginsDir", &pluginsDirFlwr);



    InitInfo meta = {initPluginsStack(100)};
    launch(&meta);


    /*CALL HOOK*/
    if(start_hook) {
        start_hook();
    }

    if(end_hook) {
        end_hook();
    }
    exitMainLoop(&meta);
    free(meta.plugins);

    return 0;
}