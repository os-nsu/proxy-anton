/*!
    \file master.h
    \brief master process life cycle
    \version 1.0
    \date 24 Oct 2024

    This file contains main loop interface and callback functions
    for work with life cycle of master process.

    IDENTIFICATION
        src/include/master.h
*/


#ifndef MASTER_H
#define MASTER_H


/*!
    \struct Plugin
    \brief Pair of dlopen handle and name
*/
struct Plugin {
    void *handle;
    char *name;
};

/*!
    \struct PluginsStack
    \brief Stack for plugin pointers
*/
struct PluginsStack {
    struct Plugin *plugins;
    int size;
    int maxIdx;
};


/*!
    \brief Initializes stack for plugins
    \param[in] bootSize Start size of stack
    \return Pointer to new stack 
*/
struct PluginsStack *initPluginsStack(int bootSize);

/*!
    \brief Free whole elements in stack
    \param[in] stack Pointer to stack
    \return 0 if success, -1 and sets -1 else;
*/
int freePlugins(struct PluginsStack *stack);

/*!
    \brief Pushes plugin to stack
    \param[in] stack Pointer to stack
    \param[in] plugin Plugin's pointer
    \param[in] name Plugin's name
    \return 0 if success, -1 else
*/
int pushPlugin(struct PluginsStack *stack, void *plugin, char *name);

/*!
    \brief pop plugin from stack
    Pops plugin from stack
    \param[in] stack Pointer to stack
    \return plugin if success, NULL else
*/
struct Plugin popPlugin(struct PluginsStack *stack);

/*!
    \brief gets plugin by it's name
    \param[in] stack Pointer to plugin stack
    \param[in] name Name of plugin
    \return plugin if success, empty plugin structure else
*/
struct Plugin getPlugin(struct PluginsStack *stack, char *name);

/*!
    \brief Master entry point
    \return 0 if success, -1 else
*/
int mainMasterLoop (void);

typedef void(*Hook)(void);

extern Hook start_hook;

extern Hook end_hook;

#endif    // MASTER_H