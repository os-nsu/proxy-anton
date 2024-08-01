/*!
    \file master.h
    \brief master process life cycle
    \version 1.0
    \date 18 Jul 2024

    This file contains main loop interface and callback functions
    for work with life cycle of master process.

    IDENTIFICATION
        src/include/master.h
*/


#ifndef MASTER_H
#define MASTER_H

typedef struct BGWorkerData {
    char *name;
    char *libName;
    char *funcName;
    int pid;
    int status;  
    struct BGWorkerData *next;
} BackgroundWorker;

#define UNDEFINED_PID -1
#define ST_REGISTERED 1
#define ST_STARTED 2
#define ST_DIED 3

typedef struct WorkersListData {
    BackgroundWorker *head;
    int size;
} WorkersList;


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


extern WorkersList backgroundWorkers;

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




typedef struct RegionNodeData {
    struct RegionNodeData *next;
    char *name;
    void *begin;
    int size;
} RegionNode;

typedef struct RegionTableData {
    RegionNode **heads;
    int size;
} RegionTable;


typedef struct SharedAreaManagementStruct {
    void *begin;
    int filledSize;
    int capacity;
} SharedAreaManager;

/*!
    \brief Initializes hash table of regions
    \return Pointer to hash table if success, -1 else
*/
RegionTable *initRegionTable(int size);

/*!
    \brief Adds pair of name and region into hahsh table
    \param[in] table Pointer to hash table
    \param[in] name Name of region
    \param[in] begin Begin of region
    \param[in] size Size of region
    \return 0 if success, -1 else
*/
int addRegion(RegionTable *table, const char *name, void *begin, int size);

/*!
    \brief Finds region in hash table by name
    \param[in] table Pointer to hash table
    \param[in] name Name of region
    \return Pointer to region node if success, NULL else
*/
RegionNode *findRegion(RegionTable *table, const char *name);


/*!
    \brief Frees table 
    \param[in] table Pointer to hash table
*/
void freeRegionTable(RegionTable *table);


/*!
    \brief Requests for extend shared memory
    \param[in] size Size of memory extension
    \return 0 if success, -1 else
*/
int requestSharedMemory(int size);


/*!
    \brief Start up shared memory
    \return Pointer to management structure of mapped area if success, NULL else
*/
SharedAreaManager *mapSharedMemory(void);

/*!
    \brief Find or register new shared area
    \param[in] manager Pointer to shared memory management structure
    \param[in] table Pointer to table of regions
    \param[in] name Name of area
    \param[in] size Size of area
    \param[out] found Result flag, 1 if region was found, 0 else
    \return Pointer to begin of area if success, NULL else
*/
void *registerSharedArea(SharedAreaManager *manager, RegionTable *table, const char *name, int size, int *found);



 

/*!
    \brief Master entry point
    \return 0 if success, -1 else
*/
int mainMasterLoop (int argc, char **argv);

typedef void(*Hook)(void);

extern Hook startMainLoopHook;

extern Hook endMainLoopHook;

typedef void (*SharedMemoryHook)(SharedAreaManager *, RegionTable *);

extern Hook sharedMemoryRequestHook;

extern SharedMemoryHook sharedMemoryStartUpHook;



#endif    // MASTER_H